Page {
    title = 'My Deva',
    Panel {
        columns = 1,
        Panel {
            columns = { 0.3, 0.7 },
            Button { 'Internal|HD drive',  page='DiskUtils' },
            Button { 'Ready' },
            Button { 'Mirror|Drive #1', page='MirrorDrive'},
            Button { 'Mirror|Status' },
        },
        Panel {
            h = 0.3,
            rows = 1,
            Panel  { },
            Button { 'Mirror Drive|Select', setting='mirror.drive' },
            Panel  { },
            Button { 'Firewire|Power', setting='mirror.firewire.power' },
            Panel  { },
        },
    },
}


