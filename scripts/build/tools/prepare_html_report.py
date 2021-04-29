#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import argparse
import shutil
import os

def parse_args():
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('input_path', help="path that contains the input htmls")
    parser.add_argument('output_path', help="output path for the final report")
    return parser.parse_args()
    

if __name__ == "__main__":
    options = parse_args()
    if not os.path.exists(options.input_path):
        raise Exception(f"Path {options.input_path} does not exist")
    
    if not os.path.exists(options.output_path):
        os.makedirs(options.output_path)
        
    for file in os.listdir(options.input_path):
        file_path = os.path.join(options.input_path, file)
        if os.path.isfile(file_path) and file_path.endswith(".html"):
            destination = os.path.join(options.output_path, file)
            shutil.copy(file_path, destination)
            print(f"copy: {os.path.abspath(file_path)} -> {os.path.abspath(destination)}")
    