project "proproperty"
	libType()
	files { 
		"src/**.c",
		"src/**.cpp",
		"src/**.h",
		"genie.lua"
	}
	defines { "BUILDING_PROPROPERTY" }
	links { "engine" }
	defaultConfigurations()

linkPlugin("proproperty")