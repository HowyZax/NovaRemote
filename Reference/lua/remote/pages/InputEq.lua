Page {
    name = 'InputEq',
    Panel {
        columns = 2,
        Panel {
            w = 0.8,
            rows = 2,
            Panel {
                h = 0.8,
                Text { 'TODO: EQ GRAPH' },
            },
            Panel {
                rows = 1,
                Button { "BAND 1" },
                Button { "BAND 2" },
                Button { "BAND 3" },
                Button { "NOTCH 1" },
                Button { "NOTCH 2" },
            },
        },
        Panel {
            columns = 1,
            Button { "LEVEL" },
            Button { "FREQ" },
            Button { "Q" },
            Spacer { h=0.4 },
        },
    }
}


