#!/bin/bash

# This script automates adding, committing, and pushing all changes.
# It will exit immediately if any command fails.
set -e

# Show the current status so you can see what's being committed
echo "--- Git Status ---"
git status
echo "------------------"
echo

# Prompt for a commit message
echo "Enter a commit message (or press Enter for 'Routine update'):"
read commitMessage

# If the user didn't enter anything, use a default message
if [ -z "$commitMessage" ]; then
  commitMessage="Routine update"
fi

echo
echo "Staging all changes..."
git add .

echo "Committing with message: \"$commitMessage\""
git commit -m "$commitMessage"

echo "Pushing to GitHub..."
git push

echo
echo "✅ Done. Changes are live on GitHub."