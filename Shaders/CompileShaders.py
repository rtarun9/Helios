import glob, os
from logging import root

if __name__ == "__main__":
    root_signature_extracted = False

    # Compile vertex shaders.
    for file in glob.glob("**/*VS.hlsl", recursive=True):
        command = "dxc -T vs_6_6 -E VsMain " + file + " -Fo " + file.split(".")[0] + ".cso"
        compile_status = os.popen(command).read()

        if compile_status:
            print(compile_status)

        if not root_signature_extracted:
            command = "dxc -T vs_6_6 -E VsMain " + file + " -extractrootsignature -Fo " + "BindlessRS.cso"
            root_signature_extracted = True
        
        compile_status = os.popen(command).read()

        if compile_status:
            print(compile_status)

    # Compile pixel shaders.
    for file in glob.glob("**/*PS.hlsl", recursive = True):
        command = "dxc -T ps_6_6 -E PsMain " + file + " -Fo " + file.split(".")[0] + ".cso"
        compile_status = os.popen(command).read()

        if compile_status:
            print(file + " " + compile_status)

    # Compile compute shaders.
    for file in glob.glob("**/*CS.hlsl", recursive = True):
        command = "dxc -T cs_6_6 -E CsMain " + file + " -Fo " + file.split(".")[0] + ".cso"
        compile_status = os.popen(command).read()

        if compile_status:
            print(compile_status)