#!/bin/bash

# *******************************************************************************
#  Author  : William Ee
#  Date    : Due 2/14/2026
#  Description: CS392 - Homework 1
#  Pledge  : I pledge my honor that I have abided by the Stevens Honor System.
# ******************************************************************************

readonly RECYCLE="$HOME/.recycle"

print_usage_stdout() {
cat <<EOF
Usage: rbin.sh [-hlp] [list of files]
   -h: Display this help;
   -l: List files in the recycle bin;
   -p: Empty all files in the recycle bin;
   [list of files] with no other flags,
        these files will be moved to the
        recycle bin.
EOF
}

check_recycle() {
  if [[ ! -d "$RECYCLE" ]]; then
    mkdir "$RECYCLE"
  fi
}

too_many_options() {
  echo "Error: Too many options enabled." >&2
  print_usage_stdout
  exit 1
}

unknown_option() {
  echo "Error: Unknown option '-$1'." >&2
  print_usage_stdout
  exit 1
}

if [[ $# -eq 0 ]]; then
  print_usage_stdout
  exit 0
fi

option_count=0
option_h=0
option_l=0
option_p=0

while getopts ":hlp" options; do
  case "$options" in
    h)
      option_h=1
      option_count=$((option_count + 1));;
    l)
      option_l=1
      option_count=$((option_count + 1));;
    p)
      option_p=1
      option_count=$((option_count + 1));;
    \?)
      unknown_option "$OPTARG";;
    :)
      unknown_option "$OPTARG";;
  esac
done

shift $((OPTIND - 1))

if [[ $option_count -gt 1 ]]; then
  too_many_options
fi

if [[ $option_count -eq 1 && $# -gt 0 ]]; then
  too_many_options
fi

if [[ $option_h -eq 1 ]]; then
  print_usage_stdout
  exit 0
fi

check_recycle

if [[ $option_l -eq 1 ]]; then
  ls -lAF "$RECYCLE"
  exit 0
fi

if [[ $option_p -eq 1 ]]; then
  rm -rf -- "$RECYCLE"/* "$RECYCLE"/.[!.]* "$RECYCLE"/..?* 2>/dev/null
  exit 0
fi

for item in "$@"; do
  if [[ -e "$item" ]]; then
    mv -- "$item" "$RECYCLE"/
  else
    echo "Warning: '$item' not found." >&2
  fi
done

exit 0
