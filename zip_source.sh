#!/bin/bash
# Compress source code (exclude build artifacts, .git, editor files)

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT="Human-Evolution-Simulation_${TIMESTAMP}.tar.gz"

tar -czf "${PROJECT_DIR}/${OUTPUT}" \
    -C "${PROJECT_DIR}" \
    --exclude='.git' \
    --exclude='build' \
    --exclude='.cache' \
    --exclude='.vscode' \
    --exclude='.idea' \
    --exclude='*.o' \
    --exclude='*.a' \
    --exclude='*.so' \
    --exclude='*.dylib' \
    --exclude='.DS_Store' \
    --exclude="${OUTPUT}" \
    --exclude='zip_source.sh' \
    .

echo "Created: ${OUTPUT} ($(du -h "${PROJECT_DIR}/${OUTPUT}" | cut -f1))"
