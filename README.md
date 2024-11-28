# **Assignment 5 - IoT Project**

This document provides instructions for setting up and running the `Assignment_5` project on the Contiki operating system. Follow the steps below to build and simulate the project.

---

## **Prerequisites**
- **Contiki Installed:** Ensure you have Contiki installed on your system.
- **Cooja Simulator:** Familiarity with Cooja simulator and Contiki development environment.
- **IoT Knowledge:** Basic understanding of IoT and wireless communication protocols.

---

## **Steps to Run the Project**

### **Step 1: Add the Source File**
Copy the `assignment_5.c` file to the `rime` folder in the Contiki `examples` directory:
```
contiki/examples/rime/
```

### **Step 2: Update the Makefile**
Edit the `Makefile` in the `rime` directory to include the `assignment_5` project. Add the following line under the `all` target:
```
all: example-abc example-mesh example-collect example-trickle example-polite \
     example-rudolph1 example-rudolph2 example-rucb assignment_5 \
     example-runicast example-unicast example-neighbors
```

Add the following line to enable C99 support:
```
CFLAGS += -std=c99
```

### **Step 3: Build and Simulate**
1. **Build the project:** Run the following command in the terminal to build for the Sky platform:
   ```
   make TARGET=sky
   ```
2. **Run the simulation:** Open the Cooja simulator, and load the `assignment_5.c` file to run the simulation.

---

## **Notes**
- The `assignment_5.c` file implements a tree-based routing algorithm and data forwarding process.
- Ensure all dependencies for Contiki and Cooja are properly installed before proceeding.

---

## **Troubleshooting**
- **Build Errors:**
  - Verify the Makefile changes are saved correctly.
  - Ensure Contiki paths are set up properly.
- **Runtime Issues:**
  - Use `CFLAGS += -std=c99` in the Makefile to enable C99 compatibility.
  - Check the terminal log output during simulation in Cooja for debugging.


