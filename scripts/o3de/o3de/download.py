#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Implements functionality for downloading o3de objects either locally or from a URI
"""

import argparse
import hashlib
import json
import logging
import os
import pathlib
import shutil
import sys
import urllib.parse
import urllib.request
import zipfile

from o3de import manifest, repo, utils, validation, register

logger = logging.getLogger()
logging.basicConfig()

def unzip_manifest_json_data(download_zip_path: pathlib.Path, zip_file_name: str) -> dict:
    json_data = {}
    with zipfile.ZipFile(download_zip_path, 'r') as zip_data:
        with zip_data.open(zip_file_name) as manifest_json_file:
            try:
                json_data = json.load(manifest_json_file)
            except json.JSONDecodeError as e:
                logger.error(f'UnZip exception:{str(e)}')

    return json_data


def validate_downloaded_zip_sha256(download_uri_json_data: dict,
                                   download_zip_path: pathlib.Path,
                                   manifest_json_name) -> int:
    # if the download_uri_json_data has a sha256 check it against a sha256 of the zip
    try:
        sha256A = download_uri_json_data['sha256']
    except KeyError as e:
        logger.warn(f'SECURITY WARNING: The advertised o3de object you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised object!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the f{manifest_json_name}.')
            return 1

    unzipped_manifest_json_data = unzip_manifest_json_data(download_zip_path, manifest_json_name)

    # remove the sha256 if present in the advertised downloadable manifest json
    # then compare it to the json in the zip, they should now be identical
    try:
        del download_uri_json_data['sha256']
    except KeyError as e:
        pass

    sha256A = hashlib.sha256(json.dumps(download_uri_json_data, indent=4).encode('utf8')).hexdigest()
    sha256B = hashlib.sha256(json.dumps(unzipped_manifest_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded manifest json does not match'
                     f' the advertised manifest json.')
        return 1

    return 0


def get_downloadable(engine_name: str = None,
                     project_name: str = None,
                     gem_name: str = None,
                     template_name: str = None,
                     restricted_name: str = None) -> dict or None:
    json_data = manifest.load_o3de_manifest()
    try:
        o3de_object_uris = json_data['repos']
    except KeyError as key_err:
        logger.error(f'Unable to load repos from o3de manifest: {str(key_err)}')
        return None

    manifest_json = 'repo.json'
    search_func = lambda manifest_json_data: repo.search_repo(manifest_json_data, engine_name, project_name, gem_name, template_name)
    o3de_object_data = repo.search_o3de_object(manifest_json, o3de_object_uris, search_func)
    if o3de_object_data:
        return o3de_object_data

    if isinstance(engine_name, str):
        if not engine_name:
            logger.error(f'engine name cannot be empty.')
            return None
        try:
            o3de_object_uris = json_data['git_engine_repos']
        except KeyError as key_err:
            logger.error(f'Unable to load git engine repos from o3de manifest: {str(key_err)}')
            return None

        manifest_json = 'engine.json'
        json_key = 'engine_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == engine_name else None
        return repo.search_o3de_object(manifest_json, o3de_object_uris, search_func)

    if isinstance(project_name, str):
        if not project_name:
            logger.error(f'project name cannot be empty.')
            return None
        try:
            o3de_object_uris = json_data['git_project_repos']
        except KeyError as key_err:
            logger.error(f'Unable to load git project repos from o3de manifest: {str(key_err)}')
            return None

        manifest_json = 'project.json'
        json_key = 'project_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == project_name else None
        return repo.search_o3de_object(manifest_json, o3de_object_uris, search_func)

    if isinstance(gem_name, str):
        if not gem_name:
            logger.error(f'gem name cannot be empty.')
            return None
        try:
            o3de_object_uris = json_data['git_gem_repos']
        except KeyError as key_err:
            logger.error(f'Unable to load git gem repos from o3de manifest: {str(key_err)}')
            return None

        manifest_json = 'gem.json'
        json_key = 'gem_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == gem_name else None
        return repo.search_o3de_object(manifest_json, o3de_object_uris, search_func)

    if isinstance(template_name, str):
        if not template_name:
            logger.error(f'template name cannot be empty.')
            return None
        try:
            o3de_object_uris = json_data['git_template_repos']
        except KeyError as key_err:
            logger.error(f'Unable to load git template repos from o3de manifest: {str(key_err)}')
            return None

        manifest_json = 'template.json'
        json_key = 'template_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == template_name else None
        return repo.search_o3de_object(manifest_json, o3de_object_uris, search_func)

    if isinstance(restricted_name, str):
        if not restricted_name:
            logger.error(f'restricted name cannot be empty.')
            return None
        try:
            o3de_object_uris = json_data['git_restricted_repos']
        except KeyError as key_err:
            logger.error(f'Unable to load git restricted repos from o3de manifest: {str(key_err)}')
            return None

        manifest_json = 'restricted.json'
        json_key = 'restricted_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == restricted_name else None
        return repo.search_o3de_object(manifest_json, o3de_object_uris, search_func)


def download_o3de_object(object_name: str, default_folder_name: str, dest_path: str or pathlib.Path,
                         object_type: str, downloadable_kwarg_key, skip_auto_register: bool) -> int:

    downloadable_object_data = get_downloadable(**{downloadable_kwarg_key : object_name})
    if not downloadable_object_data:
        logger.error(f'Downloadable o3de object {object_name} not found.')
        return 1

    found_in_repo = False
    try:
        repo_name = downloadable_object_data['repo_name']
        del downloadable_object_data['repo_name']
        o3de_object_uri = downloadable_object_data['o3de_object_uri']
        del downloadable_object_data['o3de_object_uri']
        found_in_repo = True
    except Exception:
        pass

    if found_in_repo:
        download_path = manifest.get_o3de_cache_folder()
        download_path.mkdir(parents=True, exist_ok=True)
        try:
            sha256 = downloadable_object_data["sha256"]
        except Exception as e:
            logger.error(f'Downloadable o3de object {object_name} has no sha256.')
            return 1

        download_zip_path = download_path / f'{sha256}.zip'

        try:
            origin = downloadable_object_data['origin']
        except Exception as e:
            logger.error(f'Downloadable o3de object {object_name} has no origin.')
            return 1

        url = f'{origin}/{object_type}.zip'
        parsed_uri = urllib.parse.urlparse(url)

        download_zip_result = utils.download_zip_file(parsed_uri, download_zip_path)
        if download_zip_result != 0:
            return download_zip_result

        if validate_downloaded_zip_sha256(downloadable_object_data, download_zip_path, f'{object_type}.json'):
            logger.error(f'Deleting {download_zip_path}!!!')
            os.unlink(download_zip_path)
            return 1

        if not dest_path:
            dest_path = manifest.get_registered(default_folder=default_folder_name)
            dest_path = pathlib.Path(dest_path).resolve()
            dest_path = dest_path / repo_name / object_name
        else:
            dest_path = pathlib.Path(dest_path).resolve()
        if not dest_path:
            logger.error(f'Destination path {dest_path} not cannot be empty.')
            return 1
        if dest_path.exists():
            logger.error(f'Destination path {dest_path} already exists.')
            return 1

        dest_path.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(download_zip_path, 'r') as zip_ref:
            try:
                zip_ref.extractall(dest_path)
            except Exception as e:
                logger.error(f'Unzipping {download_zip_path} to {dest_path} has failed!!! Deleting {dest_path}.')
                shutil.rmtree(dest_path)
                return 1
    else:
        try:
            o3de_object_uri = downloadable_object_data['o3de_object_uri']
            del downloadable_object_data['o3de_object_uri']
        except Exception:
            return 1

        # we found a git repo, clone/update the repo into the cache and instance it
        parsed_uri = urllib.parse.urlparse(o3de_object_uri)

        git_sha256 = hashlib.sha256(parsed_uri.geturl().encode())

        cache_folder = manifest.get_o3de_cache_folder() / str(git_sha256.hexdigest())
        if cache_folder.is_dir():
            if utils.update_git_uri(parsed_uri.geturl(), cache_folder):
                if len(os.listdir(cache_folder)) == 0:
                    cache_folder.rmdir()
                return 1
        else:
            if utils.clone_git_uri(parsed_uri.geturl(), cache_folder):
                if len(os.listdir(cache_folder)) == 0:
                    cache_folder.rmdir()
                return 1

        if not dest_path:
            dest_path = manifest.get_registered(default_folder=default_folder_name)
            dest_path = pathlib.Path(dest_path).resolve()
            dest_path = dest_path / parsed_uri.hostname
            components = parsed_uri.path.split('/')
            components = [ele for ele in components if ele.strip()]
            for ele in components:
                dest_path = dest_path / ele
        else:
            pathlib.Path(dest_path).resolve()
        if not dest_path:
            logger.error(f'Destination path {dest_path} not cannot be empty.')
            return 1
        if dest_path.exists():
            logger.error(f'Destination path {dest_path} already exists.')
            return 1

        try:
            shutil.copytree(cache_folder, dest_path)
        except Exception:
            logger.error(f'Failed to copy {cache_folder} => {dest_path}')
            shutil.rmtree(dest_path)
            return 0

    if not skip_auto_register:
        if object_type == 'project':
            return register.register(project_path=dest_path)
        elif object_type == 'gem':
            return register.register(gem_path=dest_path)
        elif object_type == 'template':
            return register.register(template_path=dest_path)
        elif object_type == 'restricted':
            return register.register(restricted_path=dest_path)
        elif object_type == 'repo':
            return register.register(repo_path=dest_path)

    return 0


def download_engine(engine_name: str,
                    dest_path: str or pathlib.Path,
                    skip_auto_register: bool) -> int:
    return download_o3de_object(engine_name, 'engines', dest_path, 'engine', 'engine_name', skip_auto_register)


def download_project(project_name: str,
                     dest_path: str or pathlib.Path,
                    skip_auto_register: bool) -> int:
    return download_o3de_object(project_name, 'projects', dest_path, 'project', 'project_name', skip_auto_register)


def download_gem(gem_name: str,
                 dest_path: str or pathlib.Path,
                skip_auto_register: bool) -> int:
    return download_o3de_object(gem_name, 'gems', dest_path, 'gem', 'gem_name', skip_auto_register)


def download_template(template_name: str,
                      dest_path: str or pathlib.Path,
                        skip_auto_register: bool) -> int:
    return download_o3de_object(template_name, 'templates', dest_path, 'template', 'template_name', skip_auto_register)


def download_restricted(restricted_name: str,
                        dest_path: str or pathlib.Path,
                        skip_auto_register: bool) -> int:
    return download_o3de_object(restricted_name, 'restricted', dest_path, 'restricted', 'restricted_name', skip_auto_register)


def _run_download(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    if args.engine_name:
        return download_engine(args.engine_name,
                               args.dest_path,
                               args.skip_auto_register)
    elif args.project_name:
        return download_project(args.project_name,
                                args.dest_path,
                               args.skip_auto_register)
    elif args.gem_name:
        return download_gem(args.gem_name,
                            args.dest_path,
                               args.skip_auto_register)
    elif args.template_name:
        return download_template(args.template_name,
                                 args.dest_path,
                               args.skip_auto_register)
    elif args.restricted_name:
        return download_restricted(args.restricted_name,
                                 args.dest_path,
                               args.skip_auto_register)

    return 1

def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python download.py --engine-name "o3de"
    :param parser: the caller passes an argparse parser like instance to this method
    """
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-e', '--engine-name', type=str, required=False,
                       help='Downloadable engine name.')
    group.add_argument('-p', '--project-name', type=str, required=False,
                       help='Downloadable project name.')
    group.add_argument('-g', '--gem-name', type=str, required=False,
                       help='Downloadable gem name.')
    group.add_argument('-t', '--template-name', type=str, required=False,
                       help='Downloadable template name.')
    group.add_argument('-r', '--restricted-name', type=str, required=False,
                       help='Downloadable restricted name.')
    parser.add_argument('-dp', '--dest-path', type=str, required=False,
                       default=None,
                       help='Optional destination folder to download into.'
                             ' i.e. download --project-name "AstomSamplerViewer" --dest-path "C:/projects"'
                             ' will result in C:/projects/AtomSampleViewer'
                             ' If blank will download to default object type folder')
    parser.add_argument('-sar', '--skip-auto-register', action='store_true', required=False,
                        default=False,
                        help='Skip Automatic registration of the new object instance.')
    parser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                        help='By default the home folder is the user folder, override it to this folder.')

    parser.set_defaults(func=_run_download)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py download --engine-name "o3de"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    download_subparser = subparsers.add_parser('download')
    add_parser_args(download_subparser)


def main():
    """
    Runs download.py script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers

    # add args to the parser
    add_parser_args(the_parser)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # return
    sys.exit(ret)


# Do not allow running the download.py script as a standalone script until it is reviewed by app-sec
