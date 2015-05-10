
-- premake gets a tiny bit confused if the same project appears in multiple
-- solutions in a single run. premake adds a bogus $projectname path to the
-- intermediate objects directory in that case. work-around using multiple
-- invocations of premake and a custom option to distinguish them.

MPT_PREMAKE_VERSION = ""

if _PREMAKE_VERSION == "4.3" then
 MPT_PREMAKE_VERSION = "4.3"
elseif _PREMAKE_VERSION == "4.4-beta5" then
 MPT_PREMAKE_VERSION = "4.4"
elseif _PREMAKE_VERSION == "5.0-alpha3" then
 MPT_PREMAKE_VERSION = "5.0"
else
 print "Premake 4.3 or 4.4-beta5 or 5.0-alpha3 required"
 os.exit(1)
end

newoption {
 trigger     = "group",
 value       = "PROJECTS",
 description = "OpenMPT project group",
 allowed = {
  { "libopenmpt-all", "libopenmpt-all" },
  { "libopenmpt_test", "libopenmpt_test" },
  { "libopenmpt", "libopenmpt" },
  { "foo_openmpt", "foo_openmpt" },
  { "in_openmpt", "in_openmpt" },
  { "xmp-openmpt", "xmp-openmpt" },
  { "openmpt123", "openmpt123" },
  { "PluginBridge", "PluginBridge" },
  { "OpenMPT", "OpenMPT" },
  { "all-externals", "all-externals" }
 }
}

function replace_in_file (filename, from, to)
	local text
	local infile
	local outfile
	local oldtext
	local newtext
	infile = io.open(filename, "r")
	text = infile:read("*all")
	infile:close()
	oldtext = text
	newtext = string.gsub(oldtext, from, to)
	text = newtext
	if newtext == oldtext then
   print("Failed to postprocess '" .. filename .. "': " .. from .. " -> " .. to)
   os.exit(1)
	end
	outfile = io.open(filename, "w")
	outfile:write(text)
	outfile:close()
end

function postprocess_vs2008_mfc (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "UseOfMFC=\"2\"", "UseOfMFC=\"1\"")
end
end

function postprocess_vs2008_main (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "\t\t\t\tEntryPointSymbol=\"mainCRTStartup\"\n", "")
elseif MPT_PREMAKE_VERSION == "4.4" then
	replace_in_file(filename, "\t\t\t\tEntryPointSymbol=\"mainCRTStartup\"\n", "")
elseif MPT_PREMAKE_VERSION == "5.0" then
	replace_in_file(filename, "\t\t\t\tEntryPointSymbol=\"mainCRTStartup\"\n", "")
end
end

function postprocess_vs2008_dynamicbase (filename)
	replace_in_file(filename, "\t\t\t\tEnableCOMDATFolding=\"2\"", "\t\t\t\tEnableCOMDATFolding=\"2\"\n\t\t\t\tRandomizedBaseAddress=\"2\"")
end

function postprocess_vs2008_nonxcompat (filename)
	replace_in_file(filename, "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n", "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n\t\t\t\t\DataExecutionPrevention=\"1\"\n")
end

function postprocess_vs2008_largeaddress (filename)
	replace_in_file(filename, "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n", "\t\t\t<Tool\n\t\t\t\tName=\"VCLinkerTool\"\n\t\t\t\t\LargeAddressAware=\"2\"\n")
end

function postprocess_vs2010_mfc (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "<UseOfMfc>Dynamic</UseOfMfc>", "<UseOfMfc>Static</UseOfMfc>")
end
end

function postprocess_vs2010_main (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "<EntryPointSymbol>mainCRTStartup</EntryPointSymbol>", "")
elseif MPT_PREMAKE_VERSION == "4.4" then
	replace_in_file(filename, "<EntryPointSymbol>mainCRTStartup</EntryPointSymbol>", "")
elseif MPT_PREMAKE_VERSION == "5.0" then
	replace_in_file(filename, "<EntryPointSymbol>mainCRTStartup</EntryPointSymbol>", "")
end
end

function postprocess_vs2010_dynamicbase (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "<EnableCOMDATFolding>true</EnableCOMDATFolding>", "<EnableCOMDATFolding>true</EnableCOMDATFolding>\n\t\t\t<RandomizedBaseAddress>true</RandomizedBaseAddress>")
elseif MPT_PREMAKE_VERSION == "4.4" then
	replace_in_file(filename, "<EnableCOMDATFolding>true</EnableCOMDATFolding>", "<EnableCOMDATFolding>true</EnableCOMDATFolding>\n      <RandomizedBaseAddress>true</RandomizedBaseAddress>")
elseif MPT_PREMAKE_VERSION == "5.0" then
	replace_in_file(filename, "<EnableCOMDATFolding>true</EnableCOMDATFolding>", "<EnableCOMDATFolding>true</EnableCOMDATFolding>\n      <RandomizedBaseAddress>true</RandomizedBaseAddress>")
end
end

function postprocess_vs2010_nonxcompat (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "\t\t</Link>\n", "\t\t\t<DataExecutionPrevention>false</DataExecutionPrevention>\n\t\t</Link>\n")
elseif MPT_PREMAKE_VERSION == "4.4" then
	replace_in_file(filename, "    </Link>\n", "      <DataExecutionPrevention>false</DataExecutionPrevention>\n    </Link>\n")
elseif MPT_PREMAKE_VERSION == "5.0" then
	replace_in_file(filename, "    </Link>\n", "      <DataExecutionPrevention>false</DataExecutionPrevention>\n    </Link>\n")
end
end

function postprocess_vs2010_largeaddress (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "\t\t</Link>\n", "\t\t\t<LargeAddressAware>true</LargeAddressAware>\n\t\t</Link>\n")
elseif MPT_PREMAKE_VERSION == "4.4" then
	replace_in_file(filename, "    </Link>\n", "      <LargeAddressAware>true</LargeAddressAware>\n    </Link>\n")
elseif MPT_PREMAKE_VERSION == "5.0" then
	replace_in_file(filename, "    </Link>\n", "      <LargeAddressAware>true</LargeAddressAware>\n    </Link>\n")
end
end

function fixbug_vs2010_pch (filename)
if MPT_PREMAKE_VERSION == "4.3" then
	replace_in_file(filename, "</PrecompiledHeader>\n\t\t</ClCompile>", "</PrecompiledHeader>")
end
end

newaction {
 trigger     = "postprocess",
 description = "OpenMPT postprocess the project files to mitigate premake problems",
 execute     = function ()

  postprocess_vs2008_main("build/vs2008/libopenmpt_test.vcproj")
  postprocess_vs2008_main("build/vs2008/openmpt123.vcproj")
  postprocess_vs2008_main("build/vs2008/libopenmpt_example_c.vcproj")
  postprocess_vs2008_main("build/vs2008/libopenmpt_example_c_mem.vcproj")
  postprocess_vs2008_mfc("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_dynamicbase("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_nonxcompat("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_largeaddress("build/vs2008/OpenMPT.vcproj")
  postprocess_vs2008_dynamicbase("build/vs2008/PluginBridge.vcproj")
  postprocess_vs2008_nonxcompat("build/vs2008/PluginBridge.vcproj")
  postprocess_vs2008_largeaddress("build/vs2008/PluginBridge.vcproj")

  postprocess_vs2010_main("build/vs2010/libopenmpt_test.vcxproj")
  postprocess_vs2010_main("build/vs2010/openmpt123.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_c.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_c_mem.vcxproj")
  postprocess_vs2010_main("build/vs2010/libopenmpt_example_cxx.vcxproj")
  postprocess_vs2010_mfc("build/vs2010/in_openmpt.vcxproj")
  postprocess_vs2010_mfc("build/vs2010/xmp-openmpt.vcxproj")
  postprocess_vs2010_mfc("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2010/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2010/PluginBridge.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2010/PluginBridge.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2010/PluginBridge.vcxproj")
  fixbug_vs2010_pch("build/vs2010/OpenMPT.vcxproj")

if MPT_PREMAKE_VERSION == "5.0" then
  
	postprocess_vs2010_main("build/vs2012/libopenmpt_test.vcxproj")
  postprocess_vs2010_main("build/vs2012/openmpt123.vcxproj")
  postprocess_vs2010_main("build/vs2012/libopenmpt_example_c.vcxproj")
  postprocess_vs2010_main("build/vs2012/libopenmpt_example_c_mem.vcxproj")
  postprocess_vs2010_main("build/vs2012/libopenmpt_example_cxx.vcxproj")
  postprocess_vs2010_mfc("build/vs2012/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2012/OpenMPT.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2012/OpenMPT.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2012/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2012/PluginBridge.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2012/PluginBridge.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2012/PluginBridge.vcxproj")
	
  postprocess_vs2010_main("build/vs2013/libopenmpt_test.vcxproj")
  postprocess_vs2010_main("build/vs2013/openmpt123.vcxproj")
  postprocess_vs2010_main("build/vs2013/libopenmpt_example_c.vcxproj")
  postprocess_vs2010_main("build/vs2013/libopenmpt_example_c_mem.vcxproj")
  postprocess_vs2010_main("build/vs2013/libopenmpt_example_cxx.vcxproj")
  postprocess_vs2010_mfc("build/vs2013/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2013/OpenMPT.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2013/OpenMPT.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2013/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2013/PluginBridge.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2013/PluginBridge.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2013/PluginBridge.vcxproj")

  postprocess_vs2010_main("build/vs2015/libopenmpt_test.vcxproj")
  postprocess_vs2010_main("build/vs2015/openmpt123.vcxproj")
  postprocess_vs2010_main("build/vs2015/libopenmpt_example_c.vcxproj")
  postprocess_vs2010_main("build/vs2015/libopenmpt_example_c_mem.vcxproj")
  postprocess_vs2010_main("build/vs2015/libopenmpt_example_cxx.vcxproj")
  postprocess_vs2010_mfc("build/vs2015/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2015/OpenMPT.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2015/OpenMPT.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2015/OpenMPT.vcxproj")
  postprocess_vs2010_dynamicbase("build/vs2015/PluginBridge.vcxproj")
  postprocess_vs2010_nonxcompat("build/vs2015/PluginBridge.vcxproj")
  postprocess_vs2010_largeaddress("build/vs2015/PluginBridge.vcxproj")

end

 end
}

if _OPTIONS["group"] == "libopenmpt-all" then

solution "libopenmpt-all"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86", "x86_64" }
else
 platforms { "x32", "x64" }
end

 dofile "../../build/premake4-win/mpt-libopenmpt_test.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmpt_examples.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmptDLL.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmpt_modplug.premake4.lua"
 dofile "../../build/premake4-win/mpt-foo_openmpt.premake4.lua"
 dofile "../../build/premake4-win/mpt-in_openmpt.premake4.lua"
 dofile "../../build/premake4-win/mpt-xmp-openmpt.premake4.lua"
 dofile "../../build/premake4-win/mpt-openmpt123.premake4.lua"
 dofile "../../build/premake4-win/ext-flac.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../../build/premake4-win/ext-portaudio.premake4.lua"

end

if _OPTIONS["group"] == "libopenmpt_test" then

solution "libopenmpt_test"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86", "x86_64" }
else
 platforms { "x32", "x64" }
end

 dofile "../../build/premake4-win/mpt-libopenmpt_test.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz.premake4.lua"

end

if _OPTIONS["group"] == "foo_openmpt" then

solution "foo_openmpt"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86" }
else
 platforms { "x32" }
end

 dofile "../../build/premake4-win/mpt-foo_openmpt.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz.premake4.lua"

end

if _OPTIONS["group"] == "in_openmpt" then

solution "in_openmpt"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86" }
else
 platforms { "x32" }
end

 dofile "../../build/premake4-win/mpt-in_openmpt.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz.premake4.lua"

end

if _OPTIONS["group"] == "xmp-openmpt" then

solution "xmp-openmpt"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86" }
else
 platforms { "x32" }
end

 dofile "../../build/premake4-win/mpt-xmp-openmpt.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../../build/premake4-win/ext-pugixml.premake4.lua"

end

-- should stay the last libopenmpt solution in order to overwrite the libopenmpt base project with all possible configurations
if _OPTIONS["group"] == "libopenmpt" then

solution "libopenmpt"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86", "x86_64" }
else
 platforms { "x32", "x64" }
end

 dofile "../../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmpt_examples.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmptDLL.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmpt_modplug.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz-shared.premake4.lua"
 dofile "../../build/premake4-win/ext-portaudio.premake4.lua"

end

if _OPTIONS["group"] == "openmpt123" then

solution "openmpt123"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86", "x86_64" }
else
 platforms { "x32", "x64" }
end

 dofile "../../build/premake4-win/mpt-openmpt123.premake4.lua"
 dofile "../../build/premake4-win/mpt-libopenmpt.premake4.lua"
 dofile "../../build/premake4-win/ext-flac.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../../build/premake4-win/ext-ogg.premake4.lua"
 dofile "../../build/premake4-win/ext-portaudio.premake4.lua"

end

if _OPTIONS["group"] == "PluginBridge" then

solution "PluginBridge"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release", "ReleaseNoLTCG" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86", "x86_64" }
else
 platforms { "x32", "x64" }
end

 dofile "../../build/premake4-win/mpt-PluginBridge.premake4.lua"

end


if _OPTIONS["group"] == "OpenMPT" then

solution "OpenMPT"
 location ( "../../build/" .. _ACTION )
 configurations { "Debug", "Release", "ReleaseNoLTCG" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86", "x86_64" }
else
 platforms { "x32", "x64" }
end
 
 dofile "../../build/premake4-win/mpt-OpenMPT.premake4.lua"
 dofile "../../build/premake4-win/mpt-PluginBridge.premake4.lua"
 dofile "../../build/premake4-win/ext-flac.premake4.lua"
 dofile "../../build/premake4-win/ext-lhasa.premake4.lua"
 dofile "../../build/premake4-win/ext-minizip.premake4.lua"
 dofile "../../build/premake4-win/ext-ogg.premake4.lua"
 dofile "../../build/premake4-win/ext-portaudio.premake4.lua"
 dofile "../../build/premake4-win/ext-portmidi.premake4.lua"
 dofile "../../build/premake4-win/ext-r8brain.premake4.lua"
 dofile "../../build/premake4-win/ext-smbPitchShift.premake4.lua"
 dofile "../../build/premake4-win/ext-soundtouch.premake4.lua"
 dofile "../../build/premake4-win/ext-UnRAR.premake4.lua"
 dofile "../../build/premake4-win/ext-zlib.premake4.lua"

end

-- overwrite all external projects once again with the full matrix of possible build config combinations
if _OPTIONS["group"] == "all-externals" then

solution "all-externals"
 location ( "../../build/" .. _ACTION .. "-ext" )
 configurations { "Debug", "Release", "ReleaseNoLTCG" }
if MPT_PREMAKE_VERSION == "5.0" then
 platforms { "x86", "x86_64" }
else
 platforms { "x32", "x64" }
end

 dofile "../../build/premake4-win/ext-flac.premake4.lua"
 dofile "../../build/premake4-win/ext-lhasa.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../../build/premake4-win/ext-miniz-shared.premake4.lua"
 dofile "../../build/premake4-win/ext-minizip.premake4.lua"
 dofile "../../build/premake4-win/ext-ogg.premake4.lua"
 dofile "../../build/premake4-win/ext-portaudio.premake4.lua"
 dofile "../../build/premake4-win/ext-portmidi.premake4.lua"
 dofile "../../build/premake4-win/ext-pugixml.premake4.lua"
 dofile "../../build/premake4-win/ext-r8brain.premake4.lua"
 dofile "../../build/premake4-win/ext-smbPitchShift.premake4.lua"
 dofile "../../build/premake4-win/ext-soundtouch.premake4.lua"
 dofile "../../build/premake4-win/ext-UnRAR.premake4.lua"
 dofile "../../build/premake4-win/ext-zlib.premake4.lua"

end