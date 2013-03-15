local CFiles = { ".c", ".h" }
Build {
	Configs = {
		Config {
			Name = "generic-gcc",
			DefaultOnHost = "linux",
			Tools = { "gcc" },
		},
		Config {
			Name = "macosx-clang",
			DefaultOnHost = "macosx",
			Tools = { "clang-osx" },
		},
		Config {
			Name = "win64-msvc",
			DefaultOnHost = "windows",
			Tools = { "msvc-vs2008"; TargetPlatform = "x64" },
		},
	},
	Units = function()
		require "tundra.syntax.glob"
		Program {
			Name = "DemoBasic",
			Sources = { Glob { Dir = ".", Extensions = CFiles } },
			Libs = {
				{ "glfw"; Config = "macosx-*-*" },
			},
			Frameworks = {
				{ "OpenGL", "Cocoa"; Config = "macosx-*-*" },
			},
			Env = {
				CCOPTS = {
					-- clang and GCC
					{ "-g"; Config = { "*-gcc-debug", "*-clang-debug" } },
					{ "-g -O2"; Config = { "*-gcc-production", "*-clang-production" } },
					{ "-O3"; Config = { "*-gcc-release", "*-clang-release" } },
					{ 
						"--std=c89 -ansi -pedantic", 
						"-Werror", 
						"-Wextra", "-Wno-unused-parameter", "-Wno-unused-function"; Config = { "*-gcc-*", "*-clang-*" } },

					-- MSVC config
					{ "/MD"; Config = "*-msvc-debug" },
					{ "/MT"; Config = { "*-msvc-production", "*-msvc-release" } },
					{
						"/wd4127", -- conditional expression is constant
						"/wd4100", -- unreferenced formal parameter
						"/wd4324", -- structure was padded due to __declspec(align())
						Config = "*-msvc-*"
					},
				},
			},
		}

		Default "DemoBasic"
	end,
}
