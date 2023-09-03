/**
 * @file sa.h
 * @author Derek Huang
 * @brief C macros for simple SAL-like source annotation
 * @copyright MIT License
 */

#ifndef PDNNET_SA_H_
#define PDNNET_SA_H_

/**
 * These annotation macros are inspired by Microsoft's Source Annotation
 * Language (SAL), which provides additional information through the use of
 * macros in function declarations. One benefit is the easy disambiguation of
 * whether a pointer argument is intended for input or output, and whether or
 * not the input/output pointer is allowed to be `NULL`.
 */

/**
 * Main annotation macro.
 *
 * Typical invocation is as follows.
 *
 * `PDNNET_SA(In)`
 *    Indicates a pointer argument is an input and must be non-`NULL`.
 *
 * `PDNNET_SA(Opt(In))`
 *    Indicates a pointer argument is an input but can be `NULL`.
 *
 * `PDNNET_SA(Out)`
 *    Indicates a pointer argument is an output and must be non-`NULL`.
 *
 * `PDNNET_SA(Opt(Out))`
 *    Indicates a pointer argument is an output but can be `NULL`.
 *
 * `PDNNET_SA(In_Out)`
 *    Indicates a pointer argument is input and output. Must be non-`NULL`.
 */
#define PDNNET_SA(...)

#endif  // PDNNET_SA_H_
