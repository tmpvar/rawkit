/* ===-------- intrin.h ---------------------------------------------------===
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

#ifndef __INTRIN_H
#define __INTRIN_H

/* First include the standard intrinsics. */
#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif

#if defined(__arm__)
#include <armintr.h>
#endif

#if defined(__aarch64__)
#include <arm64intr.h>
#endif

/* For the definition of jmp_buf. */
#if __STDC_HOSTED__
#include <setjmp.h>
#endif

#endif
