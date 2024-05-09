---@diagnostic disable: inject-field
require("FSM")

---@type FSM
denethorScript = FSM:new({
	---Unique Identifier of this FSM
	---@type string
	id = "denethorScript",
	---Name of this FSM
	---@type string
	name = "Denethor Script",
	---Name of the initial state
	---@type string
	initialStateId = "",
	---States within this FSM
	---@type table<string, FSM_STATE>
	states = {
	},
})
	---Activate this FSM
function denethorScript:activate()
	self:setInitialState()
end
