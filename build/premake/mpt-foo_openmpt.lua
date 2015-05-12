
if _ACTION == "vs2010" then

 project "foo_openmpt"
  uuid "749102d3-b183-420c-a7d7-d8ff343c1a0c"
  language "C++"
  location ( "../../build/" .. _ACTION )
  objdir "../../build/obj/foo_openmpt"
  includedirs {
   "../..",
   "../../include/foobar2000sdk",
   "$(IntDir)/svn_version",
   "../../build/svn_version",
  }
  files {
   "../../libopenmpt/foo_openmpt.cpp",
  }
  flags { "Unicode" }
  links { "libopenmpt", "miniz" }
	links { "pfc", "foobar2000_SDK", "foobar2000_sdk_helpers", "foobar2000_component_client", "../../include/foobar2000sdk/foobar2000/shared/shared.lib" }
  prebuildcommands { "..\\..\\build\\svn_version\\update_svn_version_vs_premake.cmd $(IntDir)" }
  dofile "../../build/premake/premake-defaults-DLL.lua"
  dofile "../../build/premake/premake-defaults.lua"
  flags { "StaticRuntime" }

 external "pfc"
  location "../../include/foobar2000sdk/pfc"
  uuid "EBFFFB4E-261D-44D3-B89C-957B31A0BF9C"
  kind "StaticLib"
  language "C++"

 external "foobar2000_SDK"
  location "../../include/foobar2000sdk/foobar2000/SDK"
  uuid "E8091321-D79D-4575-86EF-064EA1A4A20D"
  kind "StaticLib"
  language "C++"

 external "foobar2000_sdk_helpers"
  location "../../include/foobar2000sdk/foobar2000/helpers"
  uuid "EE47764E-A202-4F85-A767-ABDAB4AFF35F"
  kind "StaticLib"
  language "C++"

 external "foobar2000_component_client"
  location "../../include/foobar2000sdk/foobar2000/foobar2000_component_client"
  uuid "71AD2674-065B-48F5-B8B0-E1F9D3892081"
  kind "StaticLib"
  language "C++"

end