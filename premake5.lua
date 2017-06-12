-- premake5.lua
workspace "lcdMenu"
   configurations { "Debug", "Release" }
   libdirs { "lib" }
   includedirs { "lib" }

project "lcdMenu"
   kind "ConsoleApp"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   files { "src/**.h", "src/**.c" }

   filter { "system:not windows" }
      links { "dl", "wiringPi", "wiringPiDev", "m", "pthread" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

-- Lua implementation of PHP scandir function
function scandir(directory)
    local i, t, popen = 0, {}, io.popen
    local pfile = popen('ls -a "'..directory..'"')
    for filename in pfile:lines() do
        i = i + 1
        t[i] = filename
    end
    pfile:close()
    return t
end

function createsharedlibary(foldername)
    project(foldername)
        kind "SharedLib"
        language "C"
        targetdir "bin/%{cfg.buildcfg}"

   files { "apps/".. foldername .. "/**.h", "apps/" .. foldername .. "/**.c" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

end

appfolders=scandir("./apps/")
for i=3,table.getn(appfolders),1 do
    print("Generate project for ", appfolders[i])
    createsharedlibary(appfolders[i])
end
