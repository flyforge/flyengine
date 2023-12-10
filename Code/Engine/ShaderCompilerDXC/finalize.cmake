# Make sure this project is built when the Editor is built
pl_add_as_runtime_dependency(ShaderCompilerDXC)

pl_add_dependency("ShaderCompiler" "ShaderCompilerDXC")