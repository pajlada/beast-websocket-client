update-deserializations: update-payloads update-messages

update-payloads:
	@./ast/venv/bin/python3 ./ast/generate-and-replace-dir.py ./src/payloads

update-messages:
	@./ast/venv/bin/python3 ./ast/generate-and-replace-dir.py ./src/messages
