#!/usr/bin/env bash

######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : build-generics
# @created     : Sunday Mar 02, 2025 00:03:10 CST
#
# @description : read .csv file of generic template jobs
######################################################################

input="$1"
while IFS=, read -r source_file template_string_func template_string_type type_names 
do
	IFS='|' read -ra type_name <<< "$type_names"
	for T in "${type_name[@]}"; do
		friendly_name="$(echo "$T" | tr ' ' '_')"
		friendly_name="$(echo "$friendly_name" | tr '*' '_ptr')"
		source_file_out="$(dirname "$source_file")/$(basename "${source_file%.*}")_${friendly_name}.${source_file##*.}"
		echo "templating '$source_file' with '$T' >> '$source_file_out'"

		if [ -f "$source_file" ]; then
			cp "$source_file" "$source_file_out"
			# replace all comments starting with "// wT "
			sed -i "s/\/\/ $template_string_type //g" "$source_file_out"

			sed -i "s/$template_string_type/$T/g" "$source_file_out"
			sed -i "s/$template_string_func/$friendly_name/g" "$source_file_out"

		else
			echo "file doesn't exist: $source_file"
			exit 1
		fi
	done
done < <(tail -n +2 "$input")
