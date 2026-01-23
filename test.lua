local main = {}

setmetatable(main, {
	__call = function(t, config)
		for key, value in pairs(config) do
			t[key] = value
		end
	end,
})

function testEventHandler(event)
	if event.data then
	end
end

function messagesFunc(event)
	print("Message received:" .. tostring(event.message) .. "\n")
end

main({
	mainRegistry = mainRegistry,

	update = function()
		local eventDispatcher = EventDispatcher(false)
		local connect = eventDispatcher:addHandler(ScriptEvent, testEventHandler)
		eventDispatcher:addHandler(MessageEvent, messagesFunc)
		eventDispatcher:dispatchEvent(ScriptEvent({ data = "ce faci" }))
		print("\nHas script event handlers? " .. tostring(eventDispatcher:hasHandlers(ScriptEvent)))
		eventDispatcher:dispatchEvent(MessageEvent("test"))
	end,

	render = function() end,
})

main.update()
main.render()
