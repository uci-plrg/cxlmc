SEGFAULT_SIGNALS=all \
SEGFAULT_OUTPUT_NAME=./stack_trace \
LD_PRELOAD=/lib/x86_64-linux-gnu/libSegFault.so \
"$@"
