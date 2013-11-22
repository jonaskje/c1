local CFiles = { ".c", ".h" }
Build {
  Env = {
    CXXOPTS = {
      "/W4",
      { "/O2"; Config = "*-vs2013-release" },
    },
    GENERATE_PDB = {
      { "0"; Config = "*-vs2013-release" },
      { "1"; Config = { "*-vs2013-debug", "*-vs2013-production" } },
    }
  },

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
			Name = "win64-vs2013",
			DefaultOnHost = "windows",
			Tools = { { "msvc-vs2013";  TargetArch = "x64" } },
		},
	},

 IdeGenerationHints = {
    Msvc = {
      -- Remap config names to MSVC platform names (affects things like header scanning & debugging)
      PlatformMappings = {
        ['win64-vs2013'] = 'x64',
        ['win32-vs2013'] = 'Win32',
      },
      -- Remap variant names to MSVC friendly names
      VariantMappings = {
        ['release']    = 'Release',
        ['debug']      = 'Debug',
        ['production'] = 'Production',
      },
    },

    -- Override output directory for sln/vcxproj files.
    MsvcSolutionDir = 'vs2013',
  },


	Units = function()
		require "tundra.syntax.glob"
		Program {
			Name = "DemoBasic",
			Sources = { Glob { Dir = ".", Extensions = CFiles } },
			Libs = {
				{ "glfw"; Config = "macosx-*-*" },
				{ "glfw3.lib", "OpenGL32.lib", "Gdi32.lib", "User32.lib", "Kernel32.lib"; Config = "win64-*-*" },
			},
			Frameworks = {
				{ "OpenGL", "Cocoa"; Config = "macosx-*-*" },
			},
			Env = {
				CPPPATH = {
					"c:/External/include"; Config = "win64-*-*",
				},
				LIBPATH = {
					"c:/External/lib"; Config = "win64-*-*",
				},

				
				--PROGOPTS = { "/MACHINE:X64"; Config = "win64-*-*" },
				CCOPTS = {
					-- clang and GCC
					{ "-g"; Config = { "*-gcc-debug", "*-clang-debug" } },
					{ "-g -O2"; Config = { "*-gcc-production", "*-clang-production" } },
					{ "-g -O3"; Config = { "*-gcc-release", "*-clang-release" } },
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
