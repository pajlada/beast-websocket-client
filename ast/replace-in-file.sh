#!/usr/bin/env bash

_header_file="$1"
_source_file="$2"

if [ -z "$_header_file" ]; then
    >&2 echo "usage: $0 <path-to-header-file> <path-to-source-file>"
    exit 1
fi

if [ -z "$_source_file" ]; then
    >&2 echo "usage: $0 <path-to-header-file> <path-to-source-file>"
    exit 1
fi

readarray -t files < <(cat -)
resulting_header_file="${files[0]}"
resulting_source_file="${files[1]}"

if [ -z "$resulting_header_file" ] || [ -z "$resulting_source_file" ]; then
    >&2 echo "No good header or source files found, $resulting_header_file - $resulting_source_file"
    exit 1
fi

deserialization_start_comment="$(awk '/\/\/ DESERIALIZATION DEFINITION START/{print NR}' "$_header_file")"
if [ -z "$deserialization_start_comment" ]; then
    >&2 echo "File '$_header_file' does not have a deserialization start marker"
    exit 0
fi
deserialization_start=$((deserialization_start_comment + 1))
deserialization_end="$(awk '/\/\/ DESERIALIZATION DEFINITION END/{print NR}' "$_header_file")"
if [ -z "$deserialization_end" ]; then
    >&2 echo "File '$_header_file' does not have a deserialization end marker"
    exit 0
fi
deserialization_end=$((deserialization_end - 1))

if [ "$deserialization_end" -ge "$deserialization_start" ]; then
    sed -i "${deserialization_start},${deserialization_end}d" "$_header_file"
fi


sed -i "${deserialization_start_comment}r $resulting_header_file" "$_header_file"

deserialization_start_comment="$(awk '/\/\/ DESERIALIZATION IMPLEMENTATION START/{print NR}' "$_source_file")"
if [ -z "$deserialization_start_comment" ]; then
    >&2 echo "File '$_source_file' does not have a deserialization start marker"
    exit 0
fi
deserialization_start=$((deserialization_start_comment + 1))
deserialization_end="$(awk '/\/\/ DESERIALIZATION IMPLEMENTATION END/{print NR}' "$_source_file")"
if [ -z "$deserialization_end" ]; then
    >&2 echo "File '$_source_file' does not have a deserialization end marker"
    exit 0
fi
deserialization_end=$((deserialization_end - 1))

if [ "$deserialization_end" -ge "$deserialization_start" ]; then
    sed -i "${deserialization_start},${deserialization_end}d" "$_source_file"
fi


sed -i "${deserialization_start_comment}r $resulting_source_file" "$_source_file"
