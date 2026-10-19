#!/bin/bash
set -e
task_dir="$(cd "$(dirname "$0")" && pwd)"
echo "Universal Controller Bridge - experimental PS5 title installer"
echo "Enable FTP in etaHEN and keep your PS5 and Mac on the same network."
if ! command -v python3 >/dev/null 2>&1; then
  echo "Python 3 is needed for this installer. Alternatively, copy PPSA73001 using PS5 Upload."
  read -r -p "Press Return to close. " task_answer
  exit 1
fi
if python3 "$task_dir/install_ucb.py"; then task_status=0; else task_status=$?; fi
read -r -p "Press Return to close. " task_answer
exit "$task_status"
