import sys, time
for i in range(0, 101, 10):
    sys.stdout.write(f"\rprogress {i:3d}%")
    sys.stdout.flush()
    time.sleep(0.1)
print("\nDONE")
