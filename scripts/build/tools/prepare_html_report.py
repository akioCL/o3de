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


"""
    pytest-html doesn't support merging reports, this utility parses the generated html
    and merges them into a single html file.
"""

import argparse
import shutil
import os
import datetime
from lxml import html

def parse_args():
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('report_name', help="name of the report")
    parser.add_argument('input_path', help="path that contains the input htmls")
    parser.add_argument('output_path', help="output path for the final report")
    return parser.parse_args()
    
def merge_reports(report_filename, path, files):
    if len(files) == 0:
        print("Warning: html no files found to merge")
        return
        
    all_tests_html = html.parse(files[0])
    all_tests_table = all_tests_html.xpath("//table[@id='results-table']")[0]
    for f in files[1:]:
        cur_html = html.parse(f)
        rows = cur_html.xpath("//tbody[contains(@class, 'results-table-row')]")
        for row in rows:
            all_tests_table.append(row)
    
    # Update number of tests passed/skipped/etc
    all_tests_html.xpath("//h1")[0].text = f"{report_filename}.html"
    
    results = []
    sum_secs = 0
    for row in all_tests_html.xpath("//tbody[contains(@class, 'results-table-row')]"):
        results.append(row.xpath(".//td[@class='col-result']")[0].text)
        sum_secs += float(row.xpath(".//td[@class='col-duration']")[0].text)
    
    time_str = str(datetime.timedelta(seconds=sum_secs)).split(".")[0]
    all_tests_html.xpath("//body/p")[1].text = f"{len(results)} tests ran in {time_str}"
    passed  = sum(1 for r in results if r == "Passed")
    skipped = sum(1 for r in results if r == "Skipped")
    failed  = sum(1 for r in results if r == "Failed")
    errors  = sum(1 for r in results if r == "Error")
    xfails  = sum(1 for r in results if r == "XFailed")
    xpasses = sum(1 for r in results if r == "XPassed")
    
    def setup_filter(clsname, sufix, value):
        all_tests_html.xpath(f".//span[@class='{clsname}']")[0].text = f"{value} {sufix}"
        input_elem = all_tests_html.xpath(f".//input[@data-test-result='{clsname}']")[0]
        if value > 0:
            try:
                del input_elem.attrib["disabled"]
            except KeyError:
                pass
        else:
            input_elem.set("disabled")
        
        
    setup_filter('passed', 'passed', passed)
    setup_filter('skipped', 'skipped', skipped)
    setup_filter('failed', 'failed', failed)
    setup_filter('error', 'errors', errors)
    setup_filter('xfailed', 'unexpected failures', xfails)
    setup_filter('xpassed', 'unexpected passes', xpasses)
    
    with open(f"{os.path.join(path, report_filename)}.html", "wb") as f:
        f.write(html.tostring(all_tests_html, encoding='utf-8'))
    
    
if __name__ == "__main__":
    options = parse_args()
    if not os.path.exists(options.input_path):
        raise Exception(f"Path {options.input_path} does not exist")
    
    if not os.path.exists(options.output_path):
        os.makedirs(options.output_path)
      
    processed_files = []      
    for f in os.listdir(options.input_path):
        file_path = os.path.join(options.input_path, f)
        if os.path.isfile(file_path) and file_path.endswith(".html"):
            destination = os.path.join(options.output_path, f)
            shutil.copy(file_path, destination)
            processed_files.append(destination)
            print(f"copy: {os.path.abspath(file_path)} -> {os.path.abspath(destination)}")
    
    merge_reports(options.report_name, options.output_path, processed_files)
    for f in processed_files:
        os.remove(f)