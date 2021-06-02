/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzTest/AzTest.h>
#include <AzTest/Platform.h>

#include "aztestrunner.h"

#include <fstream>
#include <string>

namespace AzTestRunner
{
    const int INCORRECT_USAGE = 101;
    const int LIB_NOT_FOUND = 102;
    const int SYMBOL_NOT_FOUND = 103;

    // note that MODULE_SKIPPED is not an error condition, but not 0 to indicate its not the
    // same as successfully running tests and finding them.
    const int MODULE_SKIPPED = 104;
    const char* INTEG_BOOTSTRAP = "AzTestIntegBootstrap";

    constexpr char argFromFileSeparator = '\n';

    //! display proper usage of the application
    void usage([[maybe_unused]] AZ::Test::Platform& platform)
    {
        std::stringstream ss;
        ss <<
            "AzTestRunner\n"
            "Runs AZ unit and integration tests. Exit code is the result from GoogleTest.\n"
            "\n"
            "Usage:\n"
            "   AzTestRunner.exe <lib> (AzRunUnitTests|AzRunIntegTests) [--integ] [--wait-for-debugger] [--pause-on-completion] [google-test-args]\n"
            "\n"
            "Options:\n"
            "   <lib>: the module to test\n"
            "   <hook>: the name of the aztest hook function to run in the <lib>\n"
            "           'AzRunUnitTests' will hook into unit tests\n"
            "           'AzRunIntegTests' will hook into integration tests\n"
            "   --integ: tells runner to bootstrap the engine, needed for integration tests\n"
            "            Note: you can run unit tests with a bootstrapped engine (AzRunUnitTests --integ),\n"
            "            but running integration tests without a bootstrapped engine (AzRunIntegTests w/ no --integ) might not work.\n"
            "   --wait-for-debugger: tells runner to wait for debugger to attach to process (on supported platforms)\n"
            "   --pause-on-completion: tells the runner to pause after running the tests\n"
            "   --quiet: disables stdout for minimal output while running tests\n"
            "   --args_from_file <filename>: reads additional arguments (newline separated) from the specified file (can be used in conjunction with regular command line arguments)\n"
            "\n"
            "Example:\n"
            "   AzTestRunner.exe CrySystem.dll AzRunUnitTests --pause-on-completion\n"
            "   AzTestRunner.exe CrySystem.dll AzRunIntegTests --integ\n"
            "   AzTestRunner.exe CrySystem.dll AzRunUnitTests --args_from_file=args.txt\n"
            "\n"
            "Exit Codes:\n"
            "   0 - all tests pass\n"
            "   1 - test failure\n"
            << "   " << INCORRECT_USAGE << " - incorrect usage (see above)\n"
            << "   " << LIB_NOT_FOUND << " - library/dll could not be loaded\n"
            << "   " << SYMBOL_NOT_FOUND << " - export symbol not found\n"
            << "   " << MODULE_SKIPPED << " - non-integ module was skipped (not an error)\n";

        std::cerr << ss.str() << std::endl;
    }

    //! attempt to run the int X() method exported by the specified library
    int wrapped_command_arg_main(int argc, char** argv)
    {
        AZ::Test::Platform& platform = AZ::Test::GetPlatform();

        if (argc < 3)
        {
            usage(platform);
            return INCORRECT_USAGE;
        }

        std::vector<char*> args(argc);
        for (size_t i = 0; i < argc; i++)
        {
            args[i] = argv[i];
        }
        // capture positional arguments
        // [0] is the program name
        std::string lib = args[1];
        std::string symbol = args[2];

        // shift args parameters down as we don't need lib or symbol anymore
        AZ::Test::RemoveParameters(argc, args.data(), 1, 2);

        // capture optional arguments
        bool waitForDebugger = false;
        bool isInteg = false;
        bool pauseOnCompletion = false;
        bool quiet = false;
        std::string argsFromFile;
        std::vector<std::string> fileArgs;
        for (int i = 0; i < argc; i++)
        {
            if (strcmp(args[i], "--wait-for-debugger") == 0)
            {
                waitForDebugger = true;
                AZ::Test::RemoveParameters(argc, args.data(), i, i);
                i--;
            }
            else if (strcmp(args[i], "--integ") == 0)
            {
                isInteg = true;
                AZ::Test::RemoveParameters(argc, args.data(), i, i);
                i--;
            }
            else if (strcmp(args[i], "--pause-on-completion") == 0)
            {
                pauseOnCompletion = true;
                AZ::Test::RemoveParameters(argc, args.data(), i, i);
                i--;
            }
            else if (strcmp(args[i], "--quiet") == 0)
            {
                quiet = true;
                AZ::Test::RemoveParameters(argc, args.data(), i, i);
                i--;
            }            
            else if (strcmp(args[i], "--args_from_file") == 0)
            {
                // Check that the arg file path has been passed
                if (i + 1 >= argc)
                {
                    std::cout << "Incorrect number of args_from_file arguments\n";
                    usage(platform);
                    return INCORRECT_USAGE;
                }

                std::ifstream infile(args[i + 1]);
                if (!infile.is_open())
                {
                    std::cout << "Couldn't open " << args[i + 1] << " for args input, exiting" << std::endl;
                    return INCORRECT_USAGE;
                }

                // Remove the args_from_file argument and value from the arg list
                AZ::Test::RemoveParameters(argc, args.data(), i, i + 1);
                i--;

                // Find the first empty slot in the arg list
                size_t backArg = 0;
                for (; backArg < args.size(); backArg++)
                {
                    if (args[backArg] == nullptr)
                    {
                        break;
                    }
                }

                // Construct a list of args from the args specified in the file
                std::string arg;
                while (std::getline(infile, arg, argFromFileSeparator))
                {
                    fileArgs.emplace_back(arg);
                    argc++;
                }

                // Place the args from the file in the arg list
                for (int j = 0; j < fileArgs.size(); j++)
                {
                    if (backArg < args.size())
                    {
                        // If our placement position is that of an arg in the list that was deleted we can simply reuse the slot 
                        args[backArg] = const_cast<char*>(fileArgs[j].c_str());
                    }
                    else
                    {
                        args.emplace_back(const_cast<char*>(fileArgs[j].c_str()));
                    }

                    backArg++;
                }
            }
        }
        if (quiet)
        {
            AzTestRunner::set_quiet_mode();
        }
        else
        {
            const char* cwd = AzTestRunner::get_current_working_directory();
            std::cout << "cwd = " << cwd << std::endl;

            for (int i = 0; i < argc; i++)
            {
                std::cout << "arg[" << i << "] " << args[i] << std::endl;
            }

            std::cout << "LIB: " << lib << std::endl;
        }

        // Wait for debugger
        if (waitForDebugger)
        {
            if (platform.SupportsWaitForDebugger())
            {
                std::cout << "Waiting for debugger..." << std::endl;
                platform.WaitForDebugger();
            }
            else
            {
                std::cerr << "Warning - platform does not support --wait-for-debugger feature" << std::endl;
            }
        }

        // make sure the module actually has the expected entry point before proceeding.
        // it is very expensive to start the bootstrapper.

        std::shared_ptr<AZ::Test::IFunctionHandle> testMainFunction;

        std::cout << "Loading: " << lib << std::endl;
        std::shared_ptr<AZ::Test::IModuleHandle> module = platform.GetModule(lib);

        int result = 0;
        if (module->IsValid())
        {
            std::cout << "OKAY Library loaded: " << lib << std::endl;

            testMainFunction = module->GetFunction(symbol);
            if (!testMainFunction->IsValid())
            {
                std::cerr << "FAILED to find symbol: " << symbol << std::endl;
                result = SYMBOL_NOT_FOUND;
            }
            else
            {
                std::cout << "OKAY Symbol found: " << symbol << std::endl;
            }
        }
        else
        {
            std::cerr << "FAILED to load library: " << lib << std::endl;
            result = LIB_NOT_FOUND;
        }

        // bail out early if the module does not contain the expected entry point or the module could not be loaded.
        if (result != 0)
        {
            module.reset();

            if ((isInteg) && (result == SYMBOL_NOT_FOUND))
            {
                // special case:  It is not required to put an INTEG test inside every DLL - so if
                // we failed to find the INTEG entry point in this DLL, its not an error.
                // its only an error if we find it and there are no tests, or we find it and tests actually
                // fail.
                std::cerr << "INTEG module has no entry point and will be skipped: " << lib << std::endl;
                return MODULE_SKIPPED;
            }

            return result;
        }

        platform.SuppressPopupWindows();

        // Grab a bootstrapper library if requested
        std::shared_ptr<AZ::Test::IModuleHandle> bootstrap;
        if (isInteg)
        {
            bootstrap = platform.GetModule(INTEG_BOOTSTRAP);
            if (!bootstrap->IsValid())
            {
                std::cerr << "FAILED to load bootstrapper" << std::endl;
                return LIB_NOT_FOUND;
            }

            // Initialize the bootstrapper
            auto init = bootstrap->GetFunction("Initialize");
            if (init->IsValid())
            {
                int initResult = (*init)();
                if (initResult != 0)
                {
                    std::cerr << "Bootstrapper Initialize failed with code " << initResult << ", exiting" << std::endl;
                    return initResult;
                }
            }
        }


        // run the test main function.
        if (testMainFunction->IsValid())
        {
            result = (*testMainFunction)(argc, args.data());
            std::cout << "OKAY " << symbol << "() returned " << result << std::endl;
            testMainFunction.reset();
        }

        // unload and reset the module here, because it needs to release resources that were used / activated in
        // system allocator / etc.
        module.reset();

        // Shutdown the bootstrapper
        if (bootstrap)
        {
            auto shutdown = bootstrap->GetFunction("Shutdown");
            if (shutdown->IsValid())
            {
                int shutdownResult = (*shutdown)();
                if (shutdownResult != 0)
                {
                    std::cerr << "Bootstrapper shutdown failed with code " << shutdownResult << ", exiting" << std::endl;
                    return shutdownResult;
                }
            }
            bootstrap.reset();
        }

        if (pauseOnCompletion)
        {
            AzTestRunner::pause_on_completion();
        }

        return result;
    }


    int wrapped_main(int argc/*=0*/, char** argv/*=nullptr*/)
    {
        if (argc>0 && argv!=nullptr)
        {
            return wrapped_command_arg_main(argc, argv);
        }
        else
        {
            return 0;
        }
    }
}
