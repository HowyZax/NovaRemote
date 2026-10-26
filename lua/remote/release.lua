package.path=package.path .. ';../?.lua'
--package.cpath=package.cpath .. ';../modules/?.dll'
local config = require 'config'
local file = require 'pl.file'
local path = require 'pl.path'

-------------------------------------------------------------------
local opt = {
    zipfile = 'zaxrem-'.. config.version .. '.zip',
    --zipfile = 'zaxrem-'.. config.version .. '-' .. os.date('%Y%m%d') .. '.zip',
    qtmajor = 5,
    luaversion = '53',
    files = {
        { 'release/remote.exe' },
        --{ '../modules/socket.dll' },
        --{ '../modules/struct.dll' },
        --{ '../modules/lfs.dll' },
        --{ '../modules/unicode.dll' },
        { '../gui/release/zaxhub.exe' },
        { '../gui/release/zaxrx12.exe' },
        --{ '../pdfviewer/pdf/PDFXCviewAx.dll' },
        --{ '../pdfviewer/pdf/PDFXCview.exe' },
        --{ '../pdfviewer/pdf/resource.dat' },
        --{ '../pdfviewer/install.bat' },
        --{ '../pdfviewer/uninstall.bat' },
        { 'stylesheet.css' },
        --{ 'Test - Farmland - Shooting Script.fdx' },
        --{ '../bonjour/bonjoursdksetup-v3.0.exe' },
        --{ '../footpedal/hidapi-0.7.0/windows/Release/hidapi.dll' },
        { '../recorder/libmp3lame.dll' },
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

        { 'touch+hub.cmd' },
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
    bonjour = true,
    multimedia = true,
    serial = true,
}
-------------------------------------------------------------------
local projects = {
    { 'remote', '.' },
    { 'gui', '../gui' },
}

for _,prj in ipairs(projects) do
    local name, subdir = table.unpack(prj)
    local exe = path.join(subdir, 'release', name .. '.exe')
    local prouser = path.join(subdir, name .. '.pro.user')    --- Might not exist if not building outside Qt Creator
    print('Check project', name, exe, prouser)
--[[
    print('file.modified_time(',exe,') =', file.modified_time(exe))
    print('os.time() =', os.time())
    local delta = os.time() - file.modified_time(exe)
    local MaxAllowedDelta = 20*60 -- allow a 20 minute difference in time

    print('os.time()-file.modified_time =',delta/60,' minutes')
    --print('file.modified_time(exe) =', file.modified_time(exe), 'os.time() - 2*3600', file.modified_time(prouser), 'exe - os-xx =',file.modified_time(exe) - (os.time() - 2*3600) )

    assert( (delta<MaxAllowedDelta), name .. ' build is more than MaxAllowedDelta seconds stale; rebuild the Release configuration first')

    if path.isfile(prouser) then
        
        --for line in io.open(prouser):lines() do
        --    assert(not line:match('^.*ActiveBuildConfiguration.*0.*'),
        --            name .. ' Qt project is not configured for Release build; rebuild the Release configuration first')
        --end
        
        print('mod_time(exe) =', file.modified_time(exe), 'mod_time(prouser)', file.modified_time(prouser), 'exe - prouser =',file.modified_time(exe) - file.modified_time(prouser))
        assert(file.modified_time(exe) > file.modified_time(prouser),
               name .. ' Release build is older than the configuration file; rebuild the Release configuration first')
    end
--]]
end


print 'skipped Create file: zaxhub.exe'
--[[
--file.delete('../gui/release/zaxhub.exe')
file.copy('../gui/release/gui.exe', '../gui/release/zaxhub.exe')
assert(file.modified_time('../gui/release/gui.exe') ==
        file.modified_time('../gui/release/zaxhub.exe'),
           "zaxhub.exe & gui.exe must have the same date")

print 'Create file: zaxrx12.exe'
--file.delete('../gui/release/zaxrx12.exe')
file.copy('../gui/release/gui.exe', '../gui/release/zaxrx12.exe')
assert(file.modified_time('../gui/release/gui.exe') ==
        file.modified_time('../gui/release/zaxrx12.exe'),
           "zaxrx12.exe & gui.exe must have the same date")
--]]
-------------------------------------------------------------------
-------------------------------------------------------------------

loadfile('../release.lua')(opt)

