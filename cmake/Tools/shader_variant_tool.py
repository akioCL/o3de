import json
import sys
import shutil
import os

def main(name, shader_name, str, path, rhi_header_path):

    # Map each name to the type
    map = {}
    f = open(path + '/Shaders/azslc_outputs/' + shader_name + '.options.json')
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
    if not os.path.exists(path + "/Shaders/variant_headers"):
        os.mkdir(path + "/Shaders/variant_headers")
        
    file = open(path + "/Shaders/variant_headers/" + name + "-header.hlsl.in", "w")
    file.write(add_to_file)
    file.close()

    concat_file = [rhi_header_path, path + "/Shaders/variant_headers/" + name + "-header.hlsl.in", path + '/Shaders/azslc_outputs/' + shader_name +".hlsl"]
    
    if not os.path.exists(path + "/Shaders/final_hlsl"):
        os.mkdir(path + "/Shaders/final_hlsl")

    with open(path + "/Shaders/final_hlsl/" + name + ".hlsl", "wb") as wfd:
        for f in concat_file:
            if os.path.exists(f):
                with open(f, 'rb') as fd:
                    shutil.copyfileobj(fd, wfd)

def concat(path, file1, file2, output):
    if not os.path.exists(path + "/Shaders/prepend"):
        os.mkdir(path + "/Shaders/prepend")

    with open(output, 'wb') as wfd:
        with open(file1, 'rb') as fd:
            shutil.copyfileobj(fd, wfd)
        with open(file2, 'rb') as fd:
            shutil.copyfileobj(fd, wfd)


def add_json(name, stable_id, options, path):
    opts = options.split(";")    
    option_list = {}
    for i in range(0, len(opts)):
        temp = opts[i].split("=")
        if (len(temp) == 2):
            option_list[temp[0]] = temp[1]

    if not os.path.exists(path):
        os.mkdir(path)

    if not os.path.isfile(path + "/" + name + ".shadervariantlist"):
        shader = {}
        shader["Shader"] = name
        shader["Variants"] = []
        with open(path + "/" + name + ".shadervariantlist", 'w') as f:
            json.dump(shader, f)

    f = open(path + "/" + name + ".shadervariantlist", "r")
    data = json.load(f)
    repeat = False
    for variant in data["Variants"]:
        if variant["StableId"] == stable_id:
            repeat = True
    if not repeat:
        data["Variants"].append({
            "StableId": stable_id,
            "Options": option_list
        })
    f.close()
    f = open(path + "/" + name + ".shadervariantlist", "w")
    json.dump(data, f)
    f.close()
        
if __name__ == '__main__':
    globals()[sys.argv[1]](*sys.argv[2:])

