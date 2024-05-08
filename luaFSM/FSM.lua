
---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------
-- FSM
---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------

---Base FSM class
---@class FSM
FSM = {
    ---Name of the FSM
    ---@type string
    name = "",

    ---ID of the FSM
    ---@type string
    id = "",

    ---States of the FSM
    ---@type table<string, FSM_STATE>
    states = nil,

    ---Current state of the FSM
    ---@type FSM_STATE
    currentState = nil,

    ---Initial state of the FSM
    ---@type FSM_STATE
    initialState = nil,

    ---Name of the initial state
    ---@type string
    initialStateId = nil,

    ---Events that the FSM listens to
    ---@type table<string, boolean>
    updateEvents = nil,
} FSM.__index = FSM

------------------------------------------------
--Constructor
------------------------------------------------

---Constructor for FSM
---@param o table
---@return FSM newObject
function FSM:new(o)
    -- Make sure the fields are empty so they arent drawn from the metatable
    o = o or {
        --[[]]name = "",
        id = "",
        states = {},
        updateEvents = {},
    }
    o.states = o.states or {}
    o.updateEvents = o.updateEvents or {}
    --If this is updating an existing object, make sure the states are FSM_STATE objects
    --This will make compatible when reloading the script
    if o.states then
        for k, v in pairs(o.states) do
            if getmetatable(v) ~= FSM_STATE then
                o.states[k] = FSM_STATE:new(v)
            end
        end
    end
    if o.currentState and getmetatable(o.currentState) ~= FSM_STATE then
        o.currentState = FSM_STATE:new(o.currentState)
    end
    if o.initialState and getmetatable(o.initialState) ~= FSM_STATE then
        o.initialState = FSM_STATE:new(o.initialState)
    end
    setmetatable(o, self)
    return o
end
------------------------------------------------
--Register functions
------------------------------------------------


---Register a state with the FSM
---@param state table
function FSM:registerState(state)
    --State must have an ID and events
    if not state.id or state.id == "" or not state.events or #state.events == 0 then
        return
    end
    if not state.name or state.name == "" then state.name = state.id end

    --Create a new FSM_STATE object
    self.states[state.id] = FSM_STATE:new(state)
    self.states[state.id].FSM = self

    --Fire the onInit function of the state
    self.states[state.id]:onInit()

    --If this is the first state, set it as the initial state, so that there is always a state to start with
    --if not self.currentState then
    --    log("Setting initial state to " .. state.name, logLevel.TRACE)
    --    self.initialState = self.states[state.id]
    --    self.currentState = self.initialState
    --    self.initialState:registerEvents()
    --end
end

function FSM:setInitialState(stateId)
    if not stateId then return end
    local state = self.states[stateId]
    if not state then return end
    self.initialState = state
    self.currentState = state
    self.initialState:registerEvents()
end

--- Register events ---

---Register a state with the FSM
---@param eventName string
function FSM:registerEvent(eventName)

    callbacks[eventName][self.id] = bind(self, self.onUpdate)

    --This is used to keep track of which events the FSM is listening to
    self.updateEvents[eventName] = true
end

---Remove an event from the FSM
---@param eventName string
function FSM:removeEvent(eventName)

    if not self.updateEvents[eventName] then return end

    callbacks[eventName][self.id] = nil

    self.updateEvents[eventName] = false
end

------------------------------------------------
--FSM flow
------------------------------------------------

---Update the FSM
---@param ... unknown Variable arguments
function FSM:onUpdate(...)

    --Fire the onUpdate function of the current state, with variable arguments
    self.currentState:onUpdate(...)

    --Evaluate the triggers of the current state
    self.currentState:evaluateTriggers()
end

---Change the state of the FSM
---@param newState FSM_STATE
function FSM:changeState(newState)

    --Fire the onExit function of the current state
    if self.currentState then
        self.currentState:onExit()
    end

    --Remove the events of the previous state
    self.currentState:removeEvents()

    --Set the new state as the current state
    self.currentState = newState

    --Register the events of the new state
    self.currentState:registerEvents()

    --Fire the onEnter function of the new state
    self.currentState:onEnter()

end

------------------------------------------------
--Helper functions
------------------------------------------------

---Prints the FSM object
function FSM:debug()
    print(inspect(self))
end

---Helper function to bind a function to an object, using itself as the first argument
---@param self any
---@param func function
---@return function
function bind(self, func)
    return function(...)
        return func(self, ...)
    end
end

---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------
-- FSM State
---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------

---Base FSM State class
---@class FSM_STATE
FSM_STATE = {

    ---Name of the state
    ---@type string
    name = "",

    ---ID of the state
    ---@type string
    id = "",

    ---Description of the state
    ---@type string
    description = "",

    ---Function called when the state is initialized
    ---@type function
    onInit = function(...) end,

    ---Function called when the state is entered
    ---@type function
    onEnter = function(...) end,

    ---Function called when the state is updated
    ---@type function
    onUpdate = function(...) end,

    ---Function called when the state is exited
    ---@type function
    onExit = function(...) end,

    ---Triggers of the state (Conditions to change state)
    ---@type table<string, FSM_TRIGGER>
    triggers = nil,

    ---Data the state holds
    ---@type table<string, any>
    data = nil,

    ---FSM the state belongs to
    ---@type FSM
    FSM = nil,

    ---Events the state listens to
    ---@type table<integer, string>
    events = nil

} FSM_STATE.__index = FSM_STATE

------------------------------------------------
--Constructor
------------------------------------------------

---Constructor for FSM State
---@param o table
---@return FSM_STATE newObject
function FSM_STATE:new(o)
    -- Make sure the fields are empty so they arent drawn from the metatable
    -- And make sure the triggers are FSM_TRIGGER objects
    o = o or {
        triggers = {},
        events = {},
        data = {},
    }
    o.triggers = o.triggers or {}
    o.events = o.events or {}
    o.data = o.data or {}
    if o.triggers then
        for k, v in pairs(o.triggers) do
            if getmetatable(v) ~= FSM_TRIGGER then
                o.triggers[k] = FSM_TRIGGER:new(v)
            end
        end
    end
    if o.FSM and getmetatable(o.FSM) ~= FSM then
        setmetatable(o.FSM, FSM)
    end
    setmetatable(o, self)
    return o
end

------------------------------------------------
--Register functions
------------------------------------------------

---Register a trigger with a state
---@param trigger FSM_TRIGGER
function FSM_STATE:registerTrigger(trigger)
    if not trigger.id then return end

    --Register the trigger with the state
    self.triggers[trigger.id] = FSM_TRIGGER:new(trigger)

    --Set the current state of the trigger to this state
    trigger.currentState = self

end

---Register events ---

---Register the events of the state with the FSM
function FSM_STATE:registerEvents()

    --Loop through the events of the state and register them with the FSM
    for _, event in pairs(self.events) do
        self.FSM:registerEvent(event)
    end
end

---Remove the events of the state from the FSM
function FSM_STATE:removeEvents()

    --Loop through the events of the state and remove them from the FSM
    for _, event in pairs(self.events) do
        self.FSM:removeEvent(event)
    end
end

------------------------------------------------
--FSM State flow
------------------------------------------------

---Evaluate the triggers of the state
function FSM_STATE:evaluateTriggers()

    ---If a trigger is true, it will be stored here
    ---@type FSM_TRIGGER
    local trueTrigger = nil

    ---Keep track of the highest priority of the triggers, initialize to a very low number
    local highestPriority = -1000000

    ---Loop through the triggers and evaluate them
    for _, trigger in pairs(self.triggers) do
        if trigger:evaluate(self.data) and trigger.priority > highestPriority then
            trueTrigger = trigger
            highestPriority = trigger.priority
        end
    end

    if not trueTrigger then return end

    ---Get the next state of the true and highest priority trigger
    local nextState = trueTrigger:getNextState()

    ---Change the state of the FSM
    if nextState then
        self.FSM:changeState(nextState)
    end
end

------------------------------------------------
--Helper functions
------------------------------------------------

---Helper function to fully print the state object
function FSM_STATE:debug()
    print(inspect(self))
end

---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------
-- FSM Trigger
---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------

---Base FSM Trigger class
---@class FSM_TRIGGER
FSM_TRIGGER = {

    ---Name of the trigger
    ---@type string
    name = "",

    ---ID of the trigger
    ---@type string
    id = "",

    ---Description of the trigger
    ---@type string
    description = "",

    ---Priority of the trigger
    ---@type integer
    priority = 0,

    ---Condition to evaluate the trigger
    ---@type function
    condition = function(...) end,

    ---Function called when the trigger is true
    ---@type function
    onTrue = function(...) end,

    ---Function called when the trigger is false
    ---@type function
    onFalse = function(...) end,

    ---data for the trigger evaluation
    ---@type table<string, any>
    data = nil,

    ---ID of the next state
    ---@type string
    nextStateId = "",

    ---Next state of the trigger
    ---@type FSM_STATE
    nextState = nil,

    ---Current state of the trigger
    ---@type FSM_STATE
    currentState = nil,
} FSM_TRIGGER.__index = FSM_TRIGGER

------------------------------------------------
--Constructor
------------------------------------------------

---Constructor for FSM Trigger
---@param o FSM_TRIGGER
---@return FSM_TRIGGER
function FSM_TRIGGER:new(o)
    -- Make sure the fields are empty so they arent drawn from the metatable
    o = o or {
        data = {},
    }
    o.data = o.data or {}
    if o.currentState and getmetatable(o.currentState) ~= FSM_STATE then
        setmetatable(o.currentState, FSM_STATE)
    end
    if o.nextState and getmetatable(o.nextState) ~= FSM_STATE then
        setmetatable(o.nextState, FSM_STATE)
    end
    setmetatable(o, self)
    return o
end

------------------------------------------------
--FSM Trigger flow
------------------------------------------------

---Set the data of the trigger, getting them from the state's data
---@param data table Data passed on from the state 
function FSM_TRIGGER:setData(data)

    for k, v in pairs(self.data) do
        --Set the argument to the data from the state
        self.data[k] = data[k]
    end

end

---Evaluate the trigger
---@param data table Data passed on from the state
---@return boolean isTrue
function FSM_TRIGGER:evaluate(data)

    --Set the data of the trigger
    self:setData(data)

    --Evaluate the condition of the trigger
    if self:condition() then
        self:onTrue()
        return true
    else
        self:onFalse()
        return false
    end
end

------------------------------------------------
--Helper functions
------------------------------------------------

---Get the next state of the trigger, if it hasn't been set yet
---There is both a string and an object so that you can declare triggers without having to have made the state object yet
---@return FSM_STATE
function FSM_TRIGGER:getNextState()
    if not self.nextState then
        self.nextState = self.currentState.FSM.states[self.nextStateId]
    end
    return self.nextState
end

---Helper function to fully print the trigger object
function FSM_TRIGGER:debug()
    print(inspect(self))
end