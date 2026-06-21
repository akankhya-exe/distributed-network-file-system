mkdir tests
cat > tests/test_basic.sh << 'EOF'
#!/bin/bash

# start SS and NM in background
../../build/ss &
SS_PID=$!
sleep 1

../../build/nm &
NM_PID=$!
sleep 1

# run client and capture output
OUTPUT=$(echo "files/hello.txt" | ../../build/client)

# check if we got something back
if echo "$OUTPUT" | grep -q "hello"; then
    echo "PASS: file served successfully"
else
    echo "FAIL: did not receive file contents"
fi

# cleanup
kill $SS_PID $NM_PID 2>/dev/null
EOF

chmod +x tests/test_basic.sh