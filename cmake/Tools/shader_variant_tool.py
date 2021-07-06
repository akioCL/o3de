import json
import sys
import shutil
import os.path

def main(name, str, path, rhi_header_path):

    # Map each name to the type
    map = {}
    f = open(path + '/Shaders/azslc_outputs/' + name + '.options.json')
    options = json.load(f)
    for i in options["ShaderOptions"]:
        map[i["name"]] = i["values"]

    opts = str.split(";")
    add_to_file = "\n"
    for i in range(0, len(opts)):
        temp = opts[i].split("=")
        # Ensure formatting is correct
        if (len(temp) == 2):
            add_to_file += "#define " + temp[0] + "_OPTION_DEF " + temp[1] + "\n"
            if temp[1] not in map[temp[0]]:
                add_to_file = "\n"
                break
                #MAYBE instead you want this to throw an error instead

    file = open(path + "/Shaders/variant_headers/" + name + "-header.hlsl.in", "w")
    file.write(add_to_file)
    file.close()

    concat_file = [rhi_header_path, path + "/Shaders/variant_headers/" + name + "-header.hlsl.in", path + '/Shaders/azslc_outputs/' + name +".hlsl"]

    with open(path + "/Shaders/final_hlsl/" + name + ".hlsl", "wb") as wfd:
        for f in concat_file:
            if os.path.exists(f):
                with open(f, 'rb') as fd:
                    shutil.copyfileobj(fd, wfd)

if __name__ == '__main__':
    main(*sys.argv[1:])

