package.path=package.path .. ';../?.lua'
--package.cpath=package.cpath .. ';../modules/?.dll'
local config = require 'config'
local file = require 'pl.file'
local path = require 'pl.path'
local osdir = require 'pl.dir'

local ext = '.app'		-- extension for executable

-------------------------------------------------------------------
local opt = {
    --zipfile = 'zaxrem-'.. config.version .. '.zip',
    --zipfile = 'zaxrem-'.. config.version .. '-' .. os.date('%Y%m%d') .. '.zip',
    --qtmajor = 5,
    --luaversion = '53',
    files = {
        --{ 'release/remote.exe' },
        --{ '../modules/socket.dll' },
        --{ '../modules/struct.dll' },
        --{ '../modules/lfs.dll' },
        --{ '../modules/unicode.dll' },
        --{ '../gui/release/zaxhub.exe' },
        --{ '../gui/release/zaxrx12.exe' },
        --{ '../pdfviewer/pdf/PDFXCviewAx.dll' },
        --{ '../pdfviewer/pdf/PDFXCview.exe' },
        --{ '../pdfviewer/pdf/resource.dat' },
        --{ '../pdfviewer/install.bat' },
        --{ '../pdfviewer/uninstall.bat' },
        { 'stylesheet.css' },
        --{ 'Test - Farmland - Shooting Script.fdx' },
        --{ '../bonjour/bonjoursdksetup-v3.0.exe' },
        --{ '../footpedal/hidapi-0.7.0/windows/Release/hidapi.dll' },
        --{ '../recorder/libmp3lame.dll' },
        --{ '../../tools/DbgView.exe' },
--      { 'license-tablet.zip' },
--      { 'license-oasis.zip' },
--      { 'license-deva32.zip' },
        { '../protocol.h' },
        { '../ProtocolStrings.h' },

        { '../fonts/digital-7 (mono italic).ttf', 'fonts/' },
        { '../fonts/digital-7 (mono).ttf',        'fonts/' },
        { '../fonts/digital-7.ttf',               'fonts/' },
        { '../fonts/digital-7 (italic).ttf',      'fonts/' },
        { '../fonts/digital-7 (italic).ttf',      'fonts/' },

        { '../images/backspace.svg',   'images/' },
        { '../images/battery.svg',     'images/' },
        { '../images/dante.svg',       'images/' },
        { '../images/down.svg',        'images/' },
        { '../images/fader-black.svg', 'images/' },
        { '../images/fader-white.svg', 'images/' },
        { '../images/left.svg',        'images/' },
        { '../images/logo.svg',        'images/' },
        { '../images/minus.svg',       'images/' },
        { '../images/phase.svg',       'images/' },
        { '../images/plus.svg',        'images/' },
        { '../images/pulse.svg',       'images/' },
        { '../images/right.svg',       'images/' },
        { '../images/sine.svg',        'images/' },
        { '../images/smptebars.svg',   'images/' },
        { '../images/sprites.svg',     'images/' },
        { '../images/triangle.svg',    'images/' },
        { '../images/up.svg',          'images/' },
        { '../images/splash.png',      'images/' },

        --{ 'touch+hub.cmd' },
    },
    bundle = {
        {'modules'},
        --{'network'},
        --{'network', 'network'},
        {'copas', 'modules/copas'},
        {'pl', 'pl'},
        {'settings', 'settings'},
        {'remote', 'remote'},
        {'remote.pages', 'remote/pages'},
        {'ui', 'ui'},
        {'ui.core', 'ui/core'},
        {'ui.runtime', 'ui/runtime'},
        {'ui.runtime.qt', 'ui/runtime/qt'},
        {'ui.widgets', 'ui/widgets'},
        {'ui.widgets.qt', 'ui/widgets/qt'},
        {'fdviewer', 'fdviewer'},
        {'bonjour', 'bonjour'},
        {'movieslate', 'movieslate'},
        {'player', 'player'},
        {'serial', 'serial'},
        {'recorder', 'recorder'},
        {'config', 'config'},
        {'hub', 'hub'},
        {'rx12', 'rx12'},
    },
    svg = true,
    xml = true,
    network = true,
    --bonjour = true,
    multimedia = true,
    serial = true,
}
-------------------------------------------------------------------
local projects = {
    { 'remote', '.' },
    { 'gui', '../gui' },
}
if false then
	for _,prj in ipairs(projects) do
		local name, subdir = table.unpack(prj)
		local exe = path.join(subdir, 'release', name .. ext)
		local prouser = path.join(subdir, name .. '.pro.user')
		print('Check project', name, exe, prouser)
		assert(file.modified_time(exe) > os.time() - 2*60*60,
			   name .. ' build is stale; rebuild the Release configuration first')
	--    for line in io.open(prouser):lines() do
	--        --assert(not line:match('^.*ActiveBuildConfiguration.*0.*'),
	--        assert(not line:match('^.*>Debug<*'),
	--                name .. ' Qt project is not configured for Release build; rebuild the Release configuration first')
	--    end
		assert(file.modified_time(exe) > file.modified_time(prouser),
			   name .. ' Release build is older than the configuration file; rebuild the Release configuration first')

	end
end

print('Create file: zaxhub' .. ext)
--file.delete('../gui/release/zaxhub.exe')
--file.copy('../gui/release/gui'..ext , '../gui/release/zaxhub' .. ext)
osdir.clonetree('../gui/release/gui'..ext , '../gui/release/zaxhub' .. ext, osdir.copyfile, true)
if false then
	print(file.modified_time('../gui/release/gui'..ext))
	print(file.modified_time('../gui/release/zaxhub'..ext))
	assert(file.modified_time('../gui/release/gui'..ext) ==
			file.modified_time('../gui/release/zaxhub'..ext),
			   "zaxhub & gui executables must have the same date")
end
print('Create file: zaxrx12' .. ext)
--file.delete('../gui/release/zaxrx12.exe')
--file.copy('../gui/release/gui'..ext, '../gui/release/zaxrx12'..ext)
osdir.clonetree('../gui/release/gui'..ext , '../gui/release/zaxrx12' .. ext, osdir.copyfile, true)
if false then
	assert(file.modified_time('../gui/release/gui'..ext) ==
			file.modified_time('../gui/release/zaxrx12'..ext),
			   "zaxrx12 & gui executables must have the same date")
end

-------------------------------------------------------------------
-------------------------------------------------------------------

loadfile('../release-mac.lua')(opt)

