#!/bin/bash
# ============================================================================
# GridShield Code Coverage — Build, Run, Collect
# ============================================================================
#
# Usage:  cd firmware/coverage && bash run_coverage.sh
#
# Prerequisites: gcc, g++, lcov, genhtml
#   Ubuntu: sudo apt install gcc g++ lcov
#
# Output:  report/index.html
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
REPORT_DIR="${SCRIPT_DIR}/report"
THRESHOLD=80

echo ""
echo "=============================================="
echo " GridShield Code Coverage"
echo "=============================================="
echo ""

# 1. Build
echo "[1/4] Building with coverage flags..."
cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}" --parallel

# 2. Run tests
echo ""
echo "[2/4] Running tests..."
"${BUILD_DIR}/gridshield_tests" || true

# 3. Collect coverage
echo ""
echo "[3/4] Collecting coverage data..."
lcov --capture \
    --directory "${BUILD_DIR}" \
    --output-file "${BUILD_DIR}/coverage.info" \
    --rc lcov_branch_coverage=1 \
    --quiet

# Filter out test files and third-party code (micro-ecc, unity shim)
lcov --remove "${BUILD_DIR}/coverage.info" \
    '*/test_app/*' \
    '*/coverage/*' \
    '*/micro-ecc/*' \
    '/usr/*' \
    --output-file "${BUILD_DIR}/coverage_filtered.info" \
    --rc lcov_branch_coverage=1 \
    --quiet

# 4. Generate HTML report
echo ""
echo "[4/4] Generating HTML report..."
rm -rf "${REPORT_DIR}"
genhtml "${BUILD_DIR}/coverage_filtered.info" \
    --output-directory "${REPORT_DIR}" \
    --rc lcov_branch_coverage=1 \
    --title "GridShield Code Coverage" \
    --legend \
    --quiet

# Summary
echo ""
echo "=============================================="
echo " Coverage Report"
echo "=============================================="
lcov --summary "${BUILD_DIR}/coverage_filtered.info" --rc lcov_branch_coverage=1 2>&1

# Check threshold
LINE_COV=$(lcov --summary "${BUILD_DIR}/coverage_filtered.info" 2>&1 | grep "lines" | grep -oP '[\d.]+%' | head -1 | tr -d '%')
echo ""
echo "Line coverage: ${LINE_COV}%  (threshold: ${THRESHOLD}%)"

if (( $(echo "${LINE_COV} < ${THRESHOLD}" | bc -l) )); then
    echo "::error::Coverage ${LINE_COV}% is below threshold ${THRESHOLD}%"
    echo "FAIL: Coverage below ${THRESHOLD}%"
    exit 1
else
    echo "PASS: Coverage meets ${THRESHOLD}% threshold"
fi

echo ""
echo "HTML report: ${REPORT_DIR}/index.html"
echo ""
