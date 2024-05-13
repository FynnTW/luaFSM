
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
        name = "",
        id = "",
        states = {},
    }
    o.states = o.states or {}
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
    --State must have an ID
    if not state.id or state.id == "" then
        FSM_LOG:log("state not valid")
        return 
    end
    if not state.name or state.name == "" then state.name = state.id end

    --Create a new FSM_STATE object
    self.states[state.id] = FSM_STATE:new(state)
    self.states[state.id].FSM = self
    FSM_LOG:log("registering state " .. state.id, FSM_LOG.logLevel.TRACE)
end

function FSM:setInitialState(stateId)
    if not stateId then return end
    local state = self.states[stateId]
    if not state then
        FSM_LOG:log("State " .. stateId .. " not found") 
        return 
    end
    self.initialState = state
    self.currentState = state
    FSM_LOG:log("Set initial state " .. state.id, FSM_LOG.logLevel.TRACE)
    self.initialState:onEnter()
end

------------------------------------------------
--FSM flow
------------------------------------------------

---Update the FSM
---@param ... unknown Variable arguments
function FSM:onUpdate(...)

    if not self.currentState then return end
    
    FSM_LOG:log("onUpdate: " .. self.currentState.id, FSM_LOG.logLevel.TRACE)

    local oldId = self.currentState.id

    --Fire the onUpdate function of the current state, with variable arguments
    self.currentState:onUpdate(...)

    if self.currentState.isExitState then
        self:setInitialState(self.initialStateId)
        return
    end

    --Evaluate the triggers of the current state
    self.currentState:evaluateConditions()

    if oldId ~= self.currentState.id then
        self:onUpdate(...)
    end
end

---Change the state of the FSM
---@param newState FSM_STATE
function FSM:changeState(newState)

    FSM_LOG:log("Changed State to " .. newState.id, FSM_LOG.logLevel.TRACE)

    --Fire the onExit function of the current state
    if self.currentState then
        self.currentState:onExit()
    end

    --Set the new state as the current state
    self.currentState = newState

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
-- FSM Node
---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------
---Base FSM Node class
---@class FSM_NODE
FSM_NODE = {

    ---@type string
    id = "",

    ---@type string
    name = "",

    ---@type string
    description = "",

    ---@type table<number>
    editorPos = {0, 0},

    ---@type table<number>
    color = {0, 0, 0, 0},

} FSM_NODE.__index = FSM_NODE

---Constructor for FSM Node
---@param o table
---@return any newObject
function FSM_NODE:new(o)
    o = o or {
        id = "",
        name = "",
        description = "",
        editorPos = {0, 0},
        color = {0, 0, 0, 0},
    }
    setmetatable(o, FSM_NODE)
    return o
end

---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------
-- FSM State
---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------

FSM_STATE = FSM_NODE:new({})
---Base FSM State class
---@class FSM_STATE : FSM_NODE
FSM_STATE = {

    ---Function called when the state is entered
    ---@type function
    onEnter = function(...) end,

    ---Function called when the state is updated
    ---@type function
    onUpdate = function(...) end,

    ---Function called when the state is exited
    ---@type function
    onExit = function(...) end,

    ---Conditions to change state
    ---@type table<string, FSM_CONDITION>
    links = nil,

    ---If the state is an exit state
    ---@type boolean
    isExitState = nil,

    ---FSM the state belongs to
    ---@type FSM
    FSM = nil,

}
FSM_STATE.__index = FSM_STATE


------------------------------------------------
--Constructor
------------------------------------------------

---Constructor for FSM State
---@param o table
---@return FSM_STATE newObject
function FSM_STATE:new(o)
    -- Make sure the fields are empty so they arent drawn from the metatable
    -- And make sure the conditionds are FSM_CONDITION objects
    o = o or {
        links = {},
    }
    o.links = o.links or {}
    if o.links then
        for k, v in pairs(o.links) do
            if getmetatable(v) ~= FSM_CONDITION then
                o.links[k] = FSM_CONDITION:new(v)
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
---@param condition FSM_CONDITION
function FSM_STATE:registerCondition(condition)
    if not condition.id then FSM_LOG:log("condition not valid") return end

    --Register the trigger with the state
    self.links[condition.id] = FSM_CONDITION:new(condition)

    --Set the current state of the trigger to this state
    condition.currentStateId = self.id
    condition.currentState = self
    FSM_LOG:log("registering conditon " .. condition.id, FSM_LOG.logLevel.TRACE)
end

------------------------------------------------
--FSM State flow
------------------------------------------------

---Sort the conditions of the state by priority
---@return table<FSM_CONDITION>
function FSM_STATE:sortConditions()
    local sorted = {}
    local i = 1
    for _, condition in pairs(self.links) do
        sorted[i] = condition
        i = i + 1
    end
    table.sort(sorted, function(a, b)
        return a.priority > b.priority
    end)
    return sorted
end

---Evaluate the conditions of the state
function FSM_STATE:evaluateConditions()
    local sorted = self:sortConditions()
    for _, condition in pairs(sorted) do
        if condition:evaluate() then
            condition:action()
            if condition:getNextState() then
                self.FSM:changeState(condition:getNextState())
                if condition:getNextState().isExitState then
                    return
                end
            end
        end
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
-- FSM Condition
---------------------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------------------

FSM_CONDITION = FSM_NODE:new({})
---Base FSM Condition class
---@class FSM_CONDITION : FSM_NODE
FSM_CONDITION = {

    ---Priority of the condition
    ---@type integer
    priority = 0,

    ---Condition to evaluate the condition
    ---@type function
    condition = function(...) end,

    ---Action that executes when the condition is true
    ---@type function
    action = function(...) end,

    ---ID of the next state
    ---@type string
    nextStateId = "",

    inLineCurve = 0,
    
    outLineCurve = 0,

    ---ID of the current state
    ---@type string
    currentStateId = "",

    ---Next state of the condition
    ---@type FSM_STATE
    nextState = nil,

    ---Current state of the condition
    ---@type FSM_STATE
    currentState = nil,
} 
FSM_CONDITION.__index = FSM_CONDITION

------------------------------------------------
--Constructor
------------------------------------------------

---Constructor for FSM condition
---@param o table
---@return FSM_CONDITION newObject
function FSM_CONDITION:new(o)
    -- Make sure the fields are empty so they arent drawn from the metatable
    o = o or {
    }
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
--FSM Condition flow
-----------------------------------------------

---Evaluate the condition
---@return boolean isTrue
function FSM_CONDITION:evaluate()
    FSM_LOG:log("Evaluating " .. self.id, FSM_LOG.logLevel.TRACE)
    --Evaluate the condition of the condition
    return self:condition()
end

------------------------------------------------
--Helper functions
------------------------------------------------

---Get the next state of the Condition, if it hasn't been set yet
---There is both a string and an object so that you can declare conditions without having to have made the state object yet
---@return FSM_STATE
function FSM_CONDITION:getNextState()
    if not self.nextState then
        self.nextState = self.currentState.FSM.states[self.nextStateId]
    end
    return self.nextState
end

---Helper function to fully print the Condition object
function FSM_CONDITION:debug()
    print(inspect(self))
end



------------------------------------------------
-- LOGGING
------------------------------------------------

FSM_LOG = {
    ---@type file*
    logFile = nil,
    
    ---@enum logLevel
    logLevel = {
        TRACE = 1,
        INFO = 2,
        WARNING = 3,
        ERROR = 4
    },

    logLevelStrings = {
        "TRACE",
        "INFO",
        "WARNING",
        "ERROR"
    },

    enabled = false,

    currentLevel = 2
}

---Start the log file.
---@param path string Path to where you want the log file to be.
function FSM_LOG:start(path)
    self.logFile = io.open(path, "w+")
    self.enabled = true
end

---Set the logging level
---@param level logLevel
function FSM_LOG:setLevel(level)
    self.currentLevel = level
end

---Log something to the log. 
---@param text any
---@param level? integer
function FSM_LOG:log(text, level)
    if not self.enabled then return end
    if not self.logFile then return end
    level = level or 1
    if level < self.currentLevel then return end

    local printText = "[" .. self.logLevelStrings[level] .. "]" .. "[" .. os.date() .. "] -> <" .. tostring(text) .. ">\n"
    self.logFile:write(printText)
    self.logFile:flush()
end




