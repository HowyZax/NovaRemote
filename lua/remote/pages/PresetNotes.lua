local NUM_PRESETS = 20
local _M = Page {
    name = 'PresetNotes',
    title = "Preset Notes",
    popup = true,
    exclusive = true,
    delegate = true,
    Panel {
        Panel { -- TODO:
            h = 0.8,
            columns = { 0.7, 0.3 },
            Text { fit={10}, id='Note', tag=1, setting='note.preset'}, Text { fit={4}, id='Note', tag=2 , setting='note.preset'},
            Text { fit={10}, id='Note', tag=3, setting='note.preset'}, Text { fit={4}, id='Note', tag=4 , setting='note.preset'},
            Text { fit={10}, id='Note', tag=5, setting='note.preset'}, Text { fit={4}, id='Note', tag=6 , setting='note.preset'},
            Text { fit={10}, id='Note', tag=7, setting='note.preset'}, Text { fit={4}, id='Note', tag=8 , setting='note.preset'},
            Text { fit={10}, id='Note', tag=9, setting='note.preset'}, Text { fit={4}, id='Note', tag=10, setting='note.preset' },
        },
        Panel {
            rows = 1,
            LatchedButton { 'Edit Notes', id='Edit', highlight='blue' },
            Button { graphic='up', id='Up' },
            Button { graphic='down', id='Down' },
            Button { 'Back', id='Cancel' },
        },
    },
}

function _M:onCreated(page)
    self._top = 1
    self._notes = { }
end

function _M:onEnterPage(page, ...)
    _, self._callback = ...   -- fn
    assert(type(self._callback) == 'function')
    self.ui.edit.checked = false
end

function _M:onClickCancel()
    self:done()
end

function _M:onIndexNote(c)
    return self._top-1 + c.tag
end

function _M:onClickUp(button)
    if self._top > 2 then self._top = self._top - 2 end
    self:update()
end

function _M:onClickDown(button)
    if self._top < NUM_PRESETS - 10 then
        self._top = self._top + 2
        self:update()
    end
end

function _M:onTouchReleaseNote(c)
    if not self.ui.edit.checked and self._callback then
       print ('Complete', c:value() )
        self._callback(c:value())
        self:done()
        return true
    else
        c.setting:edit(c,c)
    end

end
