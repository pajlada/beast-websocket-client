update-deserializations: update-payloads update-messages

update-payloads: src/payloads/*.hpp
	@for header_file in $^; do \
		source_file="$$(echo $${header_file} | sed 's/.hpp/.cpp/')" ; \
		if [ ! -f "$${source_file}" ]; then \
			echo "No source file for $${header_file} found at $${source_file}" ; \
			exit 1 ; \
		fi ; \
		echo "Update deserialization code for $${header_file}/$${source_file}" ; \
		./ast/venv/bin/python3 ./ast/main.py $${header_file} | ./ast/replace-in-file.sh $${header_file} $${source_file} ; \
	done

update-messages: src/messages/*.hpp
	@for header_file in $^; do \
		source_file="$$(echo $${header_file} | sed 's/.hpp/.cpp/')" ; \
		if [ ! -f "$${source_file}" ]; then \
			echo "No source file for $${header_file} found at $${source_file}" ; \
			exit 1 ; \
		fi ; \
		echo "Update deserialization code for $${header_file}/$${source_file}" ; \
		./ast/venv/bin/python3 ./ast/main.py $${header_file} | ./ast/replace-in-file.sh $${header_file} $${source_file} ; \
	done
