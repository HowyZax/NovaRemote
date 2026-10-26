Page {
    title = "Memory",
    delegate = true,
    Panel {
        Spacer { h=0.05 },
        Panel {
            background = true,
            margin = 0.02,
            Label  { "Save to memory", align='center', fit=24, font=0.7 },
            Matrix { columns = 8, id='Save' }
        },
        Spacer { h=0.05 },
        Panel {
            background = true,
            margin = 0.02,
            Label  { "Load from memory", align='center', fit=24, font=0.7 },
            Matrix { columns = 8, id='Load' },
        },
        Spacer { h=0.1 },
        Button { h=0.15, margin={0.3,0,0.3, 0}, "Restore Factory|Defaults", id='Reset' },
    },
    onUpdateSaveCell = function(self, cell)
        cell:setText(tostring(cell.col))
    end,
    onUpdateLoadCell = function(self, cell)
        cell:setText(tostring(cell.col))
    end,
    onClickSaveCell = function(self, cell)
        SettingsSave(cell.col)
    end,
    onClickLoadCell = function(self, cell)
        SettingsLoad(cell.col)
    end,
    onClickReset = function(self, button)
        SettingsLoad('defaults')
    end,
}


