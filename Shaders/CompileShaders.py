import glob, os, enum
from msilib.schema import Complus

def compile_and_log_status(command, file):
    compile_status = os.popen(command).read()

    if compile_status:
        print(file, " -> ", compile_status)

class ShaderTypes(enum.Enum):
    none = 0
    vertex = 1
    pixel = 2
    compute = 3
    vertexAndPixel = 4

if __name__ == "__main__":
    root_signature_extracted = False

    # Compile shaders.
    for file in glob.glob("**/*.hlsl", recursive=True):
        shaderType = ShaderTypes.none

        # If the file has no VS, PS, or CS before the ., it is assumed to be a shader which has both PsMain and VsMain in the same file.
        if file.find("VS") == -1 and file.find("PS") == -1 and file.find("CS") == -1:
            shaderType = ShaderTypes.vertexAndPixel

            command = "dxc -HV 2021 -T vs_6_6 -E VsMain " + file + " -Fo " + file.split(".")[0] + "VS.cso"
            compile_and_log_status(command, file)
            
            command = "dxc -HV 2021 -T ps_6_6 -E PsMain " + file + " -Fo " + file.split(".")[0] + "PS.cso"
            compile_and_log_status(command, file)


        # Compile files which has either VS, CS, or PS postifx.
        elif file.find("VS") != -1:
            shaderType = ShaderTypes.vertex

            command = "dxc -T vs_6_6 -E VsMain " + file + " -Fo " + file.split(".")[0] + ".cso"
            compile_and_log_status(command, file)

        elif file.find("PS") != -1:
            shaderType = ShaderTypes.pixel

            command = "dxc -T ps_6_6 -E PsMain " + file + " -Fo " + file.split(".")[0] + ".cso"
            compile_and_log_status(command, file)

        elif file.find("CS") != -1:
            shaderType = ShaderTypes.compute

            command = "dxc -T cs_6_6 -E CsMain " + file + " -Fo " + file.split(".")[0] + ".cso"
            compile_and_log_status(command, file)
            
        if not root_signature_extracted:
            if shaderType == ShaderTypes.vertexAndPixel or ShaderTypes == ShaderTypes.vertex:
                command = "dxc -T vs_6_6 -E VsMain " + file + " -extractrootsignature -Fo " + "BindlessRS.cso"
            elif shaderType == ShaderTypes.pixel:
                command = "dxc -T ps_6_6 -E PsMain " + file + " -extractrootsignature -Fo " + "BindlessRS.cso"
            elif shaderType == ShaderTypes.compute:
                continue
              
            compile_and_log_status(command, file)

            root_signature_extracted = True

    