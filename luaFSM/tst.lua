---@FSM tst
---@class tst : FSM
tst = FSM:new({})
tst.name = "Test FSM3"
tst.initialStateId = "awd"


---Activate this FSM
function tst:activate()
	self:setInitialState(self.initialStateId)
end

---@FSM_STATE awd
---@class awd : FSM_STATE
local awd = FSM_STATE:new({})
awd.name = "awd"
awd.description = ""
awd.editorPos = {443, 132}

---@FSM_STATE adwaf
---@class adwaf : FSM_STATE
local adwaf = FSM_STATE:new({})
adwaf.name = "afwawf"
adwaf.description = ""
adwaf.editorPos = {-1, -1}

