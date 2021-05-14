"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
# -------------------------------------------------------------------------

import click
import os
import main as app_main


@click.version_option('1.0.0')
@click.option('--output', default='PBR', help='Lumberyard material type. Current options: [pbr_basic]')
@click.argument('operands', type=click.STRING, nargs=-1)
@click.command(context_settings=dict(ignore_unknown_options=True))
def main(output, operands):
    target_files = []
    for index, operand in enumerate(operands):
        entry_path = os.path.abspath(str(operand))
        if os.path.isdir(entry_path):
            for directory_path, directory_names, file_names in os.walk(entry_path):
                for file_name in file_names:
                    if is_valid_file(file_name):
                        target_files.append(os.path.join(entry_path, file_name))
        else:
            if is_valid_file(operand):
                target_files.append(operand)

    if len(target_files):
        app_main.launch_cli(output, target_files)


def is_valid_file(file_name):
    target_extensions = 'ma mb fbx blend max'.split(' ')
    if file_name.split('.')[-1] in target_extensions:
        return True
    return False


if __name__ == '__main__':
    main()

