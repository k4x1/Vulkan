import os

env = Environment()

# To clean build files: scons -c
# To build debug:       scons
# To build release:     scons release=1

env.Append(BUILD_DIR='bin')

is_release = ARGUMENTS.get('release', '')
target_name = 'vkcube.x64'
source_path = 'src'
shaders_path = 'shaders'

##################################
# Setup shader compilation builder
shaders = []

# collects all shader files it finds in shaders path
def enumerate_shaders():
	for root, dirs, files in os.walk(shaders_path):
		for f in files:
			if f.endswith('.vert') or f.endswith('.frag') or f.endswith('glsl'):
				sp = os.path.join(root, f)
				shaders.append(sp)

def compile_shader(target, source, env):
	shader = str(source[0])
	spirv = str(target[0])
	cmd = f"glslc {shader} -o {spirv}"
	return os.system(cmd)

enumerate_shaders()
print("Enumerated " + str(len(shaders)) + " shaders in path")

# Add builder for shader compilation
shader_builder = Builder(action=compile_shader)
env.Append(BUILDERS={'Shader': shader_builder})

# Compile all shaders
for sf in shaders:
	print("  Compiling shader: " + str(sf))
	target = sf + '.spv'
	env.Shader(sf + '.spv', sf)
	print("    compiled to: " + target)

##################################
# Setup compiler options
if env['PLATFORM'] == 'win32':
	env.Append(tools = ['msvc'])
	env.Append(CXXFLAGS = '/std:c++20 /D "WIN32" /D "_WINDOWS" /D VK_USE_PLATFORM_WIN32_KHR')
	if is_release:
		env.Append(CXXFLAGS = '/MD /O2 /EHsc /D "NDEBUG"')
		env.Append(LINKFLAGS = '')
		target_name += '.exe'
	else:
		env.Append(CXXFLAGS = '/DEBUG /MDd /Zi /EHsc')
		env.Append(LINKFLAGS = '/DEBUG')
		target_name += '.debug.exe'
else:
	env.Append(CXXFLAGS = '-std=c++20 -pedantic-errors')
	env.Append(CPPDEFINES = ['VK_USE_PLATFORM_XCB_KHR'])
	if is_release:
		env.Append(CXXFLAGS = '-O2')
	else:
		env.Append(CXXFLAGS = '-g3 -O0 -Wall')
		env.Append(LINKFLAGS = '-g')
		target_name += '.debug'

# Includes
env.Append(CPPPATH = [
	'../_Dependencies',
	'../_Dependencies/STB',
	'../_Dependencies/GLM',
	'../_Dependencies/VULKAN',
])

# Source
source = []
for cf in os.listdir(source_path):
	if cf.endswith('cpp'):
		source.append(os.path.join(source_path, cf))

# Libs
libraries = []
if env['PLATFORM'] == 'win32':
	env.Append(LIBPATH = [
		'../_Dependencies/GLFW',
	])
	libraries.append([
		'winmm.lib',
		'kernel32.lib',
		'user32.lib',
		'gdi32.lib',
		'winspool.lib',
		'shell32.lib',
		'ole32.lib',
		'oleaut32.lib',
		'uuid.lib',
		'comdlg32.lib',
		'advapi32.lib',
		'glfw3',
	])
else:
	libraries.append([
		'volk',
		'xcb',
		'glfw',
	])
env.Append(LIBS=libraries)

##################################
# Perform build
target = os.path.join(env['BUILD_DIR'], target_name)

env.Program(target=target, source=source)

if not os.path.exists(env['BUILD_DIR']):
	os.makedirs(env['BUILD_DIR'])
