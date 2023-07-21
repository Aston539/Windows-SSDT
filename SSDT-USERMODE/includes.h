#pragma once

/*
	Defining this macro stops windows.h including
	libraries such as winsockets which we dont want
	for this project
*/
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <stdio.h>

#include "../SSDT-KERNEL/shared.h"