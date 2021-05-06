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
    and merges them into a single html file and adds some enhancements like a loading spinner and editor.log coloring.
"""

import argparse
import shutil
import os
import datetime
import re
from html import escape
from lxml import html

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
    
    
    def format_log(log_element):
        # Get rid of inter-html element texts and move every block of text inside a <span>
        # as this makes way easier to parse the text
        t = str(log_element.text)
        log_element.text = ""
        log_element.insert(0, html.fromstring(f"<span>{t}</span></br>"))
        for i in log_element:
            if i.tail:
                t = escape(i.tail)
                log_element.insert(log_element.index(i)+1, html.fromstring(f"<span>{t}</span></br>"))
                i.tail = ""
       
        # Now that everything is in spans, enhance the log by setting special color for the files that
        # match editor.log lines. We divide it into 3 parts head, colored_log and tail
        for span in log_element.xpath(".//span"):
            editor_log_start = re.search(r".*\|Editor.log\|", str(span.text), re.IGNORECASE | re.M)
            if editor_log_start is None:
                continue
            
            if span.attrib.get("class") == "error":
                # These only have one line, don't do any parsing and just set the new class
                span.attrib["class"] = "editor_log"
            else:
                first_part = span.text[:editor_log_start.start()]
                next_part = span.text[editor_log_start.start():]
                editor_log_end = re.search(r"^(?!\|Editor.log\|).+", next_part, re.IGNORECASE | re.M)
                editor_log_end.start()
                log = next_part[:editor_log_end.start()]
                last_part = next_part[editor_log_end.start():]
                span.text = ""
                span.append(html.fromstring(f"<span>{escape(first_part)}</span></br>"))
                span.append(html.fromstring(f'<span class="editor_log">{escape(log)}</span></br>'))
                span.append(html.fromstring(f"<span>{escape(last_part)}</span></br>"))
            
    # Update number of tests passed/skipped/etc and format the log
    all_tests_html.xpath("//h1")[0].text = f"{report_filename}.html"
    
    results = []
    sum_secs = 0
    for row in all_tests_html.xpath("//tbody[contains(@class, 'results-table-row')]"):
        results.append(row.xpath(".//td[@class='col-result']")[0].text)
        sum_secs += float(row.xpath(".//td[@class='col-duration']")[0].text)
        row.xpath("./tr")[1].attrib["class"] = "collapsed"
        log = row.xpath(".//div[@class='log']")
        if log:
            format_log(log[0])
    
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
    
    # Add loading spinner and logic to hide it when loaded
    extra_css = """
    .editor_log {
      color: #3300ff
    }
    .lds-dual-ring {
      display: inline-block;
      width: 80px;
      height: 80px;
      margin-left: 50%;
    }
    .lds-dual-ring:after {
      content: " ";
      display: block;
      width: 64px;
      height: 64px;
      margin-left: -32px;
      border-radius: 50%;
      border: 6px solid #888;
      border-color: #888 transparent #888 transparent;
      animation: lds-dual-ring 1.2s linear infinite;
    }
    @keyframes lds-dual-ring {
      0% {
        transform: rotate(0deg);
      }
      100% {
        transform: rotate(360deg);
      }
    }
    """
    spinner_js = """
    document.getElementById('loading').remove();
    document.getElementById('results-table').removeAttribute('style');
    """
    spinner_html = '<div id="loading" class="lds-dual-ring"></div>'
    
    # CSS style
    all_tests_html.xpath("//head/style")[-1].text += extra_css
    all_tests_table.attrib["style"] = "display: none;"
    # JS code
    script =  all_tests_html.xpath("//body/script")[0]
    script.text = script.text.replace("resetSortHeaders();", "resetSortHeaders();\n" + spinner_js)
    # Insert it just before the result table
    all_tests_table.addprevious(html.fromstring(spinner_html))
    
    with open(f"{os.path.join(path, report_filename)}.html", "wb") as f:
        f.write(html.tostring(all_tests_html, encoding='utf-8'))
    
def parse_args():
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('report_name', help="name of the report")
    parser.add_argument('input_path', help="path that contains the input htmls")
    parser.add_argument('output_path', help="output path for the final report")
    return parser.parse_args()
    
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