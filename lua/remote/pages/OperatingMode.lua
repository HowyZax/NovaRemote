Page {
    title = 'Operating Mode',
    Panel {
        columns = 3,
        Button { 'Transport|Operation', setting='setup.transport' },
        Button { 'GPi1|Remote Roll',    setting='setup.gpi1.remoteroll' },
        Button { 'Serial|Remote Roll',  setting='setup.serial.remoteroll' },
        Button { 'Serial|Port Mode',    setting='setup.serial.mode' },
        Spacer  { },
        Button { 'Master|Output Mode',  setting='setup.master.mode' },
        Spacer  { },
        Spacer  { },
        Button { 'Command|Monitor', page='CommandMonitor' },
    }
}


