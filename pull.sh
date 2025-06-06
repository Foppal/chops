#!/bin/bash

# This script safely pulls the latest changes from the 'main' branch on GitHub.
# It will exit immediately if any command fails.
set -e

echo "--- Pulling Latest Changes from GitHub ---"
echo

# The 'git pull' command fetches changes from the remote and merges them
# into your current branch.
git pull origin main

echo
echo "✅ Done. Your local repository is now up-to-date."