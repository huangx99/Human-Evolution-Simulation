#!/bin/bash
set -e

cd "$(dirname "$0")"

echo "Pulling latest..."
git pull --ff-only

HASH=$(git rev-parse --short HEAD)
NAME="Human-Evolution-Simulation-${HASH}.zip"

echo "Packing ${NAME}..."
git archive --format=zip --prefix="Human-Evolution-Simulation/" HEAD -o "/mnt/data/codex/${NAME}"

ls -lh "/mnt/data/codex/${NAME}"
echo "Done."
