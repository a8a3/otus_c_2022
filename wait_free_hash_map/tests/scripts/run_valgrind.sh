#!/bin/bash
#valgrind --leak-check=yes --track-origins=yes ./wtf_table_ut
valgrind --tool=helgrind ./wtf_table_ut

