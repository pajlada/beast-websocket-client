update-deserializations: update-payloads update-messages

update-payloads:
	@./ast/venv/bin/python3 ./ast/generate-and-replace-dir.py ./include/twitch-eventsub-ws/payloads

update-messages:
	@./ast/venv/bin/python3 ./ast/generate-and-replace-dir.py ./include/twitch-eventsub-ws/messages
