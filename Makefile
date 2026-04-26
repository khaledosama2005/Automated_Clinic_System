CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET   = hospital
SRCS     = main.cpp Queue.cpp PriorityQueue.cpp
HDRS     = Patient.h User.h Condition.h Category.h Clinic.h Doctor.h \
           Receptionist.h HospitalSystem.h PriorityQueue.h Queue.h HASHMAP.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)
	@echo "Build successful → ./$(TARGET)"

clean:
	rm -f $(TARGET) *.o
