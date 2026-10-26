Page {
    name = 'DiskUtils',
    title = 'Internal Disk Utilities',
    popup = true,
    exclusive = true,
    Panel {
        --y = 0.08,
        columns = 3,
        Button { 'Erase|Current|Folder' },
        Button { 'Current|Folder', page='DiskFolders' },
        Button { 'Delete|Last|Segment' },
        Spacer { },
        Spacer { },
        Spacer { },
        Button { 'Format|Drive', page='DiskUtilsFormat' },
    }
}

Page {
    --name = "DiskFolders",
    title = "Disk Folders",
    popup = true,
    Panel {
        rows = 1,
        h = 1.0,
        Panel {
            columns = 1,
            w = 0.4,
            Spacer { h=0.04 },
            Button { 'EXT Drive|UNMOUNTED' },
            Spacer { h=0.04 },
            Button { 'EMPTY' },
            Button { 'EMPTY' },
            Button { 'EMPTY' },
        },
        Spacer { w=0.05 },
        Panel {
            columns = 1,
            spacing = 0.05,
            Button { '^' },
            Button { 'Page|Up' },
            Button { 'Page|Down' },
            Button { 'v' },
        },
        Spacer { w=0.1 },
        Panel {
            columns = 1,
            Button { 'Sort|Order' },
            Button { 'Disable DVD|Playback' },
            Button { 'Name|Folder' },
            Spacer { h=0.1 },
            Button { 'Erase|Folder' },
        }
    }
}


Page {
    name = 'DiskUtilsFormat',
    title = 'Format Menu',
    popup = true,
    exclusive = true,
    delegate = true,
    Panel {
        Label {
            'WARNING!',
            h = 0.15,
            align = 'center',
            style = 'warning',
        },
        Label {
            h = 0.45,
            align = 'center',
            style = 'warning',
            {
                'Formatting the hard disk',
                'will result in the loss of all folders',
                'and segments. Exit this screen',
                'if you do not want this to occur.'
            },
        },
        Panel {
            margin = 0.1,
            rows = 1,
            Button { 'Format|Drive', id='Format' },
            Spacer { },
            Button { 'Do NOT|Format|Drive', id='Cancel' },
        },
    },
    onClickFormat = function(self) self:exit() end,
    onClickCancel = function(self) self:exit() end,
}

Page {
    name = 'MirrorDrive',
    title = 'Mirror Drive',
    popup = true,
    exclusive = true,
    columns = 1,
    Panel {
        columns = 4,
        Button { 'File|Type', setting='mirror.filetype' },
        Button { 'File|Resolution', setting='mirror.resolution' },
        Button { 'Sample Rate|Conversion', setting='mirror.src' },
        Button { 'Format|Drive', page='MirrorFormat' },
        Button { 'Mirror|Mode', setting='mirror.mode' },
        Spacer { },
        Button { 'Start Seg', setting='mirror.startseg' },
        Button { 'End Seg', setting='mirror.endseg' },
    },
    Panel {
        h=0.15,
        rows = 1,
        Panel {
            columns = 1,
            Label { 'Folder to mirror:' },
            Label { '(0 segments)' },
        },
        Button { 'Mirror|Status' },
    },
    Panel {
        h = 0.5,
        columns = 1,
        Label { h=0.1, 'Tracks to mirror:' },
        Matrix {
            columns = 8,
            rows = 4,
            cellTitles = true,
            setting = 'mirror.tracks',
        },
    },
}

Page {
    name = 'MirrorFormat',
    title = 'Format Mirror Drive',
    popup = true,
    exclusive = true,
    delegate = true,
    Panel {
        Label {
            h = 0.15, align = 'center', style = 'warning',
            'CAUTION!',
        },
        Label {
            h = 0.45,
            align = 'center',
            style = 'warning',
            {
                'Formatting a mirror drive will',
                'result in the loss of all folders',
                'and segments. Exit this screen',
                'if you do not want this to occur.'
            },
        },
        Panel {
            margin = 0.1,
            rows = 1,
            Button { 'Format|Drive|UDF', id='FormatUDF' },
            Button { 'Format|Drive|FAT32', id='FormatFAT32' },
            Button { 'DO NOT|Format|Drive', id='Cancel' },
        },
    },
    onClickFormatUDF = function(self) self:exit() end,
    onClickFormatFAT32 = function(self) self:exit() end,
    onClickCancel = function(self) self:exit() end,
}


