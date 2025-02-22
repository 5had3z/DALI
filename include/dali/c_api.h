// Copyright (c) 2017-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DALI_C_API_H_
#define DALI_C_API_H_

#include <cuda_runtime_api.h>
#include <inttypes.h>
#include "dali/core/api_helper.h"
#include "dali/core/dali_data_type.h"

// Trick to bypass gcc4.9 old ABI name mangling used by TF
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle for DALI C-like API.
 *
 * @note Beware, the C API is just C-like API for handling some mangling issues and
 * it can throw exceptions.
 */
typedef struct DALIPipeline *daliPipelineHandle;

typedef enum {
  CPU = 0,
  GPU = 1
} device_type_t;

typedef enum {
  DALI_BACKEND_CPU = 0,
  DALI_BACKEND_GPU = 1,
  DALI_BACKEND_MIXED = 2
} dali_backend_t;

typedef daliDataType_t dali_data_type_t;

typedef enum {
  DALI_EXEC_IS_PIPELINED    = 1,
  DALI_EXEC_IS_ASYNC        = 2,
  DALI_EXEC_IS_SEPARATED    = 4,
  DALI_EXEC_IS_DYNAMIC      = 8,

  DALI_EXEC_SIMPLE          = 0,
  DALI_EXEC_ASYNC_PIPELINED = DALI_EXEC_IS_PIPELINED | DALI_EXEC_IS_ASYNC,
  DALI_EXEC_DYNAMIC         = DALI_EXEC_ASYNC_PIPELINED | DALI_EXEC_IS_DYNAMIC,
} dali_exec_flags_t;

#ifdef __cplusplus
constexpr dali_exec_flags_t operator|(dali_exec_flags_t x, dali_exec_flags_t y) {
    return dali_exec_flags_t(static_cast<int>(x) | static_cast<int>(y));
}

constexpr dali_exec_flags_t operator&(dali_exec_flags_t x, dali_exec_flags_t y) {
    return dali_exec_flags_t(static_cast<int>(x) & static_cast<int>(y));
}

#endif

/*
 * Need to keep that in sync with ReaderMeta from operator.h
 */
typedef struct {
  int64_t epoch_size;          // raw epoch size
  int64_t epoch_size_padded;   // epoch size with the padding at the end
  int number_of_shards;        // number of shards
  int shard_id;                // shard id of given reader
  int pad_last_batch;          // if given reader should pad last batch
  int stick_to_shard;          // if given reader should stick to its shard
} daliReaderMetadata;


/*
 * Need to keep that in sync with ExecutorMeta from executor.h
 */
typedef struct {
  char *operator_name;         // operator name, user need to free the memory
  size_t out_num;              // number of the operator outputs
  size_t *real_size;           // real size of the operator output, user need to free the memory
  size_t *max_real_size;       // the biggest size of the tensor in the batch
  size_t *reserved;            // reserved size of the operator output, user need to free the memory
  size_t *max_reserved;        // the biggest reserved memory size for the tensor in the batch
} daliExecutorMetadata;


typedef struct daliExternalContextField {
  char *data;
  size_t size;
} daliExternalContextField;

/*
 * Need to keep that in sync with ExternalContextCheckpoint from checkpoint.h
 */
typedef struct {
  daliExternalContextField pipeline_data;
  daliExternalContextField iterator_data;
} daliExternalContextCheckpoint;


/**
 * @brief DALI initialization
 *
 * Call this function to initialize DALI backend. It shall be called once per process.
 * Along with this, you'll need to call @see daliInitOperatorsLib() function from
 * `operators.h` file, to initialize whole DALI.
 * In the unlikely event you'd like to use only Pipeline and Executor (no Operators),
 * you may pass on calling @see daliInitOperatorsLib()
 */
DLL_PUBLIC void daliInitialize();

/**
 * @name Create DALI Pipeline via deserialization.
 * @{
 */
/**
 * @brief Create DALI pipeline. Setting max_batch_size,
 * num_threads or device_id here overrides
 * values stored in the serialized pipeline.
 * When separated_execution is equal to 0, prefetch_queue_depth is considered,
 * gpu_prefetch_queue_depth and cpu_prefetch_queue_depth are ignored.
 * When separated_execution is not equal to 0, cpu_prefetch_queue_depth and
 * gpu_prefetch_queue_depth are considered and prefetch_queue_depth is ignored.
 */
DLL_PUBLIC void daliCreatePipeline(daliPipelineHandle *pipe_handle, const char *serialized_pipeline,
                                   int length, int max_batch_size, int num_threads, int device_id,
                                   int separated_execution, int prefetch_queue_depth,
                                   int cpu_prefetch_queue_depth, int gpu_prefetch_queue_depth,
                                   int enable_memory_stats);

/**
 * Create a DALI Pipeline, using a pipeline that has been serialized beforehand.
 *
 * @param pipe_handle Pipeline handle.
 * @param serialized_pipeline Serialized pipeline.
 * @param length Length of the serialized pipeline string.
 * @param max_batch_size Maximum batch size.
 * @param num_threads Number of CPU threads which this pipeline uses.
 * @param device_id ID of the GPU device which this pipeline uses.
 * @param pipelined_execution If != 0, this pipeline will execute in Pipeline mode.
 * @param async_execution If != 0, this pipeline will execute asynchronously.
 * @param separated_execution If != 0, this pipeline will have different depths
 *                            of the CPU and GPU prefetching queues.
 * @param prefetch_queue_depth Depth of the prefetching queue.
 *                             If `separated_execution != 0`, this value is ignored.
 * @param cpu_prefetch_queue_depth Depth of the prefetching queue in the CPU stage.
 *                                 If `separated_execution == 0`, this value is ignored
 * @param gpu_prefetch_queue_depth Depth of the prefetching queue in the GPU stage.
 *                                 If `separated_execution == 0`, this value is ignored
 * @param enable_memory_stats Enable memory stats.
 */
DLL_PUBLIC void
daliCreatePipeline2(daliPipelineHandle *pipe_handle, const char *serialized_pipeline, int length,
                    int max_batch_size, int num_threads, int device_id, int pipelined_execution,
                    int async_execution, int separated_execution, int prefetch_queue_depth,
                    int cpu_prefetch_queue_depth, int gpu_prefetch_queue_depth,
                    int enable_memory_stats);

/**
 * Create a DALI Pipeline, using a pipeline that has been serialized beforehand.
 *
 * @param pipe_handle Pipeline handle.
 * @param serialized_pipeline Serialized pipeline.
 * @param length Length of the serialized pipeline string.
 * @param max_batch_size Maximum batch size.
 * @param num_threads Number of CPU threads which this pipeline uses.
 * @param device_id ID of the GPU device which this pipeline uses.
 * @param pipelined_execution If != 0, this pipeline will execute in Pipeline mode.
 * @param exec_flags Executor congiguration flags
 * @param cpu_prefetch_queue_depth Depth of the prefetching queue in the CPU stage.
 *                                 If `separated_execution == 0`, this value is ignored
 * @param gpu_prefetch_queue_depth Depth of the prefetching queue in the GPU stage.
 *                                 If `separated_execution == 0`, this value is ignored
 * @param enable_memory_stats Enable memory stats.
 */
DLL_PUBLIC void
daliCreatePipeline3(daliPipelineHandle *pipe_handle, const char *serialized_pipeline, int length,
                    int max_batch_size, int num_threads, int device_id,
                    dali_exec_flags_t exec_flags,
                    int prefetch_queue_depth,
                    int cpu_prefetch_queue_depth, int gpu_prefetch_queue_depth,
                    int enable_memory_stats);

/**
 * Convenient overload. Use it, if the Pipeline should inherit its parameters
 * from serialized pipeline.
 */
DLL_PUBLIC void daliDeserializeDefault(daliPipelineHandle *pipe_handle,
                                       const char *serialized_pipeline,
                                       int length);

/**
 * Checks, if the pipeline given by the string can be deserialized. It can be assumed that the
 * pipeline, which can be deserialized, is a formally valid DALI pipeline.
 *
 * @param serialized_pipeline String with the serialized pipeline.
 * @param length Length of the string.
 * @return 0, if the pipeline is serializable. 1 otherwise.
 */
DLL_PUBLIC int daliIsDeserializable(const char* serialized_pipeline, int length);
/** @} */

enum {
  DALI_ext_default = 0,
  /**
   * If memory transfer should be synchronous - applies to GPU memory
   */
  DALI_ext_force_sync = (1 << 0),

  /**
   * If provided CPU memory is page-locked
   */
  DALI_ext_pinned = (1 << 1),

  /**
   * If provided, a CUDA copy kernel will be used to feed external source instead of cudaMemcpyAsync
   * Only relevant when the input is either pinned host memory or device memory
   */
  DALI_use_copy_kernel = (1 << 2),

  /**
   * Override the `no_copy` specified for given External Source and force the data to be copied.
   */
  DALI_ext_force_copy = (1 << 3),

  /**
   * Override the `no_copy` specified for given External Source and pass the data directly to the
   * Pipeline.
   */
  DALI_ext_force_no_copy = (1 << 4),
};

/**
 * @name Input batch size information
 * @{
 */
/**
 * @brief Get the max batch size of a given pipeline.
 *
 * @param pipe_handle Pointer to pipeline handle
 * @return Max batch size
 */
DLL_PUBLIC int daliGetMaxBatchSize(daliPipelineHandle *pipe_handle);

/**
 * @brief Set the batch size for the upcoming call to `daliSetExternalInput*(...)`
 *
 * @param pipe_handle Pointer to pipeline handle
 * @param name Pointer to a null-terminated byte string with the name of the External Source
 *             to be fed
 * @param batch_size Batch size of the data
 */
DLL_PUBLIC void daliSetExternalInputBatchSize(daliPipelineHandle *pipe_handle, const char *name,
                                              int batch_size);

/**
 * Set the data_id for the upcoming call to `daliSetExternalInput*(...)`.
 *
 * The operator_name accepts the name of an input operator. Input operators are the operators,
 * that can work with `daliSetExternalInput*(...)` functions, e.g. fn.external_source or
 * fn.inputs.video.
 *
 * @param operator_name The name of the input operator to be fed.
 * @param data_id data_id which will be assigned during upcoming `daliSetExternalInput*(...)` call.
 */
DLL_PUBLIC void
daliSetExternalInputDataId(daliPipelineHandle *pipe_handle, const char *operator_name,
                           const char *data_id);

/**
 * @brief Returns how many times daliSetExternalInput on a given input before calling daliPrefetch
 *
 * @param pipe_handle The handle to the pipeline
 * @param input_name The name of the input in question
 * @return The number of calls to be made
 */
DLL_PUBLIC int
daliInputFeedCount(daliPipelineHandle *pipe_handle, const char *input_name);

/** @} */

/**
 * @name Contiguous inputs
 * @{
 */
/**
 * @brief Feed the data to ExternalSource as contiguous memory.
 *
 * When calling this function, you need to provide a CUDA stream, which will be used when
 * copying data onto GPU. This function is asynchronous, so it's your responsibility to
 * synchronize on a provided CUDA stream.
 *
 * If GPU memory is provided, it is assumed to reside on the same device that the pipeline is using.
 * See `device_id` parameter of the `daliCreatePipeline`.
 *
 * Keep in mind, that for the special case, where the data exists on the CPU and the
 * ExternalSource's Backend in also a CPU, stream is not needed - feel free to pass
 * the default stream.
 *
 * A convenience, synchronous, overload function is provided,
 * which handles the stream synchronization.
 *
 * If `daliSetExternalInputBatchSize` has been called prior to this function, given batch size
 * is assumed. Otherwise, the function will default to max batch size.
 * @see daliSetExternalInputBatchSize
 * @see daliCreatePipeline
 *
 * @param pipe_handle Pointer to pipeline handle
 * @param name Pointer to a null-terminated byte string with the name of the External Source
 *             to be fed
 * @param device Device of the supplied memory.
 * @param data_ptr Pointer to contiguous buffer containing all samples
 * @param data_type Type of the provided data
 * @param shapes Pointer to an array containing shapes of all samples concatenated one after
 *              another. Should contain batch_size * sample_dim elements.
 * @param sample_dim The dimensionality of a single sample.
 * @param layout_str Optional layout provided as a pointer to null-terminated byte string.
 *                   Can be set to NULL.
 * @param stream CUDA stream to use when copying the data onto GPU. Remember to synchronize on the
 *               provided stream.
 * @param flags Extra flags, check DALI_ext_* and DALI_use_copy_kernel flags
 */
DLL_PUBLIC void
daliSetExternalInputAsync(daliPipelineHandle *pipe_handle, const char *name,
                          device_type_t device, const void *data_ptr,
                          dali_data_type_t data_type, const int64_t *shapes,
                          int sample_dim, const char *layout_str,
                          cudaStream_t stream, unsigned int flags);

DLL_PUBLIC void
daliSetExternalInput(daliPipelineHandle *pipe_handle, const char *name,
                     device_type_t device, const void *data_ptr,
                     dali_data_type_t data_type, const int64_t *shapes,
                     int sample_dim, const char *layout_str, unsigned int flags);
/** @} */

/**
 * @name Sample inputs
 * @{
 */
/**
 * @brief Feed the data to ExternalSource as a set of separate buffers.
 *
 * When calling this function, you need to provide a CUDA stream, which will be used when
 * copying data onto GPU. This function is asynchronous, so it's your responsibility to
 * synchronize on a provided CUDA stream.
 *
 * Keep in mind, that for the special case, where the data exists on the CPU and the
 * ExternalSource's Backend in also a CPU, stream is not needed - feel free to pass
 * the default stream.
 *
 * A convenience, synchronous, overload function is provided,
 * which handles the stream synchronization.
 *
 * If `daliSetExternalInputBatchSize` has been called prior to this function, given batch size
 * is assumed. Otherwise, the function will default to max batch size.
 * @see daliSetExternalInputBatchSize
 * @see daliCreatePipeline
 *
 * @param pipe_handle Pointer to pipeline handle
 * @param name Pointer to a null-terminated byte string with the name of the External Source
 *             to be fed
 * @param device Device of the supplied memory.
 * @param data_ptr Pointer to an array containing batch_size pointers to separate Tensors.
 * @param data_type Type of the provided data
 * @param shapes Pointer to an array containing shapes of all samples concatenated one after
 *              another. Should contain batch_size * sample_dim elements.
 * @param sample_dim The dimensionality of a single sample.
 * @param layout_str Optional layout provided as a pointer to null-terminated byte string.
 *                   Can be set to NULL.
 * @param stream CUDA stream to use when copying the data onto GPU. Remember to synchronize on the
 *               provided stream.
 * @param flags Extra flags, check DALI_ext_force_sync, DALI_ext_pinned, DALI_use_copy_kernel
 */
DLL_PUBLIC void
daliSetExternalInputTensorsAsync(daliPipelineHandle *pipe_handle, const char *name,
                                 device_type_t device, const void *const *data_ptr,
                                 dali_data_type_t data_type, const int64_t *shapes,
                                 int64_t sample_dim, const char *layout_str,
                                 cudaStream_t stream, unsigned int flags);

DLL_PUBLIC void
daliSetExternalInputTensors(daliPipelineHandle *pipe_handle, const char *name,
                            device_type_t device, const void *const *data_ptr,
                            dali_data_type_t data_type, const int64_t *shapes,
                            int64_t sample_dim, const char *layout_str, unsigned int flags);
/** @} */

/**
 * @brief Get number of external inputs in the pipeline.
 *
 * @param pipe_handle Pointer to pipeline handle.
 * @return Number of inputs.
 */
DLL_PUBLIC int daliGetNumExternalInput(daliPipelineHandle *pipe_handle);

/**
 * @brief Get the name of n-th external input in the pipeline in the lexicographic order.
 *
 * Returned pointer is valid until the lifetime of the pipeline object ends.
 *
 * @param pipe_handle Pointer to pipeline handle.
 * @param n
 * @return Name of the external input.
 */
DLL_PUBLIC const char *daliGetExternalInputName(daliPipelineHandle *pipe_handle, int n);

/**
 * @brief Get the data layout required by the external input with a given name.
 * If the layout is not determined, an empty string is returned.
 *
 * Returned pointer is valid until the lifetime of the pipeline object ends.
 *
 * @param pipe_handle Pointer to pipeline handle.
 * @param name Name of the external input.
 * @return Layout string.
 */
DLL_PUBLIC const char *daliGetExternalInputLayout(daliPipelineHandle *pipe_handle,
                                                  const char *name);


/**
 * @brief Get the data type required by the external input with a given name.
 *
 * @param pipe_handle Pointer to pipeline handle.
 * @param name Name of the external input.
 * @return Data type.
 */
DLL_PUBLIC dali_data_type_t daliGetExternalInputType(daliPipelineHandle *pipe_handle,
                                                     const char *name);


/**
 * @brief Get the data number of dimensions required by the external input with a given name.
 * If the number of dimensions is not determined, -1 is returned.
 *
 * @param pipe_handle Pointer to pipeline handle.
 * @param name Name of the external input.
 * @return Number of dimensions.
 */
DLL_PUBLIC int daliGetExternalInputNdim(daliPipelineHandle *pipe_handle, const char *name);

/**
 * @brief Start the execution of the pipeline.
 */
DLL_PUBLIC void daliRun(daliPipelineHandle *pipe_handle);

/**
 * @brief Schedule initial runs to fill the buffers.
 *
 * This function should be called once, after a pipeline is created and external inputs
 * (if any) are populated the required number of times.
 * For subsequent runs, daliRun should be used.
 */
DLL_PUBLIC void daliPrefetch(daliPipelineHandle *pipe_handle);

/**
 * @brief Schedule first runs to fill buffers for Executor with UniformQueue policy.
 * @param queue_depth Ignored; must be equal to the pipeline's queue depth
 * @deprecated Use `daliPrefetch` instead
 */
DLL_PUBLIC void daliPrefetchUniform(daliPipelineHandle *pipe_handle, int queue_depth);

/**
 * @brief Schedule first runs to fill buffers for Executor with SeparateQueue policy.
 * @param cpu_queue_depth Ignored; must be equal to the pipeline's CPU queue depth
 * @param gpu_queue_depth Ignored; must be equal to the pipeline's GPU queue depth
 * @deprecated Use `daliPrefetch` instead
 */
DLL_PUBLIC void daliPrefetchSeparate(daliPipelineHandle *pipe_handle,
                                     int cpu_queue_depth, int gpu_queue_depth);

/**
 * @brief Wait until the output of the pipeline is ready.
 * Releases previously returned buffers.
 */
DLL_PUBLIC void daliOutput(daliPipelineHandle *pipe_handle);

/**
 * @brief Wait until the output of the pipeline is ready.
 * Doesn't release previously returned buffers.
 */
DLL_PUBLIC void daliShareOutput(daliPipelineHandle *pipe_handle);

/**
 * @brief Releases buffer returned by last daliOutput call.
 */
DLL_PUBLIC void daliOutputRelease(daliPipelineHandle *pipe_handle);

/**
 * @brief Returns 1 if the the output batch stored at position `n` in the pipeline can
 * be represented as dense, uniform tensor. Otherwise 0.
 *
 * This function may only be called after
 * calling Output function.
 */
DLL_PUBLIC int64_t daliOutputHasUniformShape(daliPipelineHandle *pipe_handle, int i);

/**
 * @brief Return the shape of the output tensor stored at position `n` in the pipeline.
 * Valid only if daliOutputHasUniformShape() returns 1.
 *
 * This function may only be called after
 * calling Output function.
 * @remarks Caller is responsible to 'free' the memory returned
 */
DLL_PUBLIC int64_t *daliShapeAt(daliPipelineHandle *pipe_handle, int n);

/**
 * @brief Return the type of the output tensor
 * stored at position `n` in the pipeline.
 * This function may only be called after
 * calling Output function.
 */
DLL_PUBLIC dali_data_type_t daliTypeAt(daliPipelineHandle *pipe_handle, int n);

/**
 * @brief Return the shape of the 'k' output tensor from tensor list
 * stored at position `n` in the pipeline.
 * This function may only be called after
 * calling Output function.
 * @remarks Caller is responsible to 'free' the memory returned
 */
DLL_PUBLIC int64_t *daliShapeAtSample(daliPipelineHandle *pipe_handle, int n, int k);

/**
 * @brief Return the number of tensors in the tensor list
 * stored at position `n` in the pipeline.
 */
DLL_PUBLIC size_t daliNumTensors(daliPipelineHandle *pipe_handle, int n);

/**
 * @brief Return the number of all elements in the tensor list
 * stored at position `n` in the pipeline.
 */
DLL_PUBLIC size_t daliNumElements(daliPipelineHandle *pipe_handle, int n);

/**
 * @brief Return the size of the tensor list
 * stored at position `n` in the pipeline.
 */
DLL_PUBLIC size_t daliTensorSize(daliPipelineHandle *pipe_handle, int n);

/**
 * @brief Return maximum number of dimensions from all tensors
 * from the tensor list stored at position `n` in the pipeline.
 */
DLL_PUBLIC size_t daliMaxDimTensors(daliPipelineHandle *pipe_handle, int n);

/**
 * @brief Check, what is the declared number of dimensions in the given output.
 *
 * Declared number of dimensions is a number, which user can optionally provide
 * at the pipeline definition stage.
 *
 * @param n Index of the output, at which the check is performed.
 */
DLL_PUBLIC size_t daliGetDeclaredOutputNdim(daliPipelineHandle *pipe_handle, int n);

/**
 * @brief Check, what is the declared data type in the given output.
 *
 * Declared data type is a type, which user can optionally provide
 * at the pipeline definition stage.
 *
 * @param n Index of the output, at which the check is performed.
 */
DLL_PUBLIC dali_data_type_t daliGetDeclaredOutputDtype(daliPipelineHandle *pipe_handle, int n);

/**
 * @brief Returns number of DALI pipeline outputs
 */
DLL_PUBLIC unsigned daliGetNumOutput(daliPipelineHandle *pipe_handle);

/**
 * @brief Returns a string indicating name of the output given by id.
 * @remark The returned pointer is invalidated after calling `daliDeletePipeline(pipe_handle)`.
 */
DLL_PUBLIC const char *daliGetOutputName(daliPipelineHandle *pipe_handle, int id);

/**
 * @brief Returns device_type_t indicating device backing pipeline output given by id
 */
DLL_PUBLIC device_type_t daliGetOutputDevice(daliPipelineHandle *pipe_handle, int id);


/**
 * @name Operator traces
 * @{
 */
/**
 * Checks, if given operator produced a trace with given name.
 *
 * In case the name of non-existing operator is provided,
 * the behaviour of this function is undefined.
 *
 * @return 0, if the trace with given name does not exist.
 */
DLL_PUBLIC int daliHasOperatorTrace(daliPipelineHandle *pipe_handle, const char *operator_name,
                                    const char *trace_name);

/**
 * Returns the traces of the given operator in the DALI Pipeline.
 *
 * Operator Traces is a communication mechanism with particular operators in the pipeline. For
 * more information @see operator_trace_map_t.
 *
 * User does not own the returned value. In a situation, when changing of this value is necessary,
 * user shall copy it to his own memory. The lifetime of this value ends, when the
 * daliOutputRelease() is called.
 *
 * User shall check, if the trace with given name exists (@see daliHasOperatorTrace). In case the
 * name of non-existing operator or non-existing trace is provided, the behaviour of this function
 * is undefined.
 *
 * @param operator_name Name of the operator, which trace shall be returned.
 * @param trace_name Name of the requested trace.
 * @return Operator trace.
 */
DLL_PUBLIC const char *
daliGetOperatorTrace(daliPipelineHandle *pipe_handle, const char *operator_name,
                     const char *trace_name);
/** @} */


/**
 * @brief Copy the output batch stored at position `output_idx` in the pipeline.
 * @remarks If the pipeline output is TensorList then it needs to be dense
 * @param pipe_handle Pointer to pipeline handle
 * @param dst Pointer to the destination buffer where the data will be copied
 * @param output_idx index of the pipeline output
 * @param dst_type Device type associated with the destination buffer (0 - CPU, 1 - GPU)
 * @param stream CUDA stream to use when copying the data to/from the GPU.
 * @param flags Extra flags, check DALI_ext_force_sync, DALI_use_copy_kernel
 */

DLL_PUBLIC void
daliOutputCopy(daliPipelineHandle *pipe_handle, void *dst, int output_idx, device_type_t dst_type,
               cudaStream_t stream, unsigned int flags);

/**
 * @brief Copy the samples in output stored at position `output_idx` in the pipeline
 *        to scattered memory locations.
 * @param pipe_handle Pointer to pipeline handle
 * @param dsts Pointers to the destination buffers where each sample will be copied.
 *        A nullptr dst pointer for a sample will discard that sample.
 * @param output_idx index of the pipeline output
 * @param dst_type Device type associated with the destination buffer (0 - CPU, 1 - GPU)
 * @param stream CUDA stream to use when copying the data to/from the GPU.
 * @param flags Extra flags, check DALI_ext_force_sync, DALI_use_copy_kernel
 */
DLL_PUBLIC void daliOutputCopySamples(daliPipelineHandle *pipe_handle, void **dsts, int output_idx,
                                      device_type_t dst_type, cudaStream_t stream,
                                      unsigned int flags);

/**
 * @brief DEPRECATED API: use daliOutputCopy instead
 */
DLL_PUBLIC void
daliCopyTensorNTo(daliPipelineHandle *pipe_handle, void *dst, int n, device_type_t dst_type,
                  cudaStream_t stream, int non_blocking);

/**
 * @brief DEPRECATED API: use daliOutputCopy instead
 */
DLL_PUBLIC void
daliCopyTensorListNTo(daliPipelineHandle *pipe_handle, void *dst, int output_id,
                      device_type_t dst_type, cudaStream_t stream, int non_blocking);

/**
 * @brief Delete the pipeline object.
 */
DLL_PUBLIC void daliDeletePipeline(daliPipelineHandle *pipe_handle);

/**
 * @brief Load plugin library
 */
DLL_PUBLIC void daliLoadLibrary(const char *lib_path);

/**
 * @brief The plugin paths will have the following pattern:
 *        {lib_path}/{sub_path}/libdali_{plugin_name}.so
 */
DLL_PUBLIC void daliLoadPluginDirectory(const char* plugin_dir);

/**
 * @brief Load default plugin library
 * @remarks DALI_PRELOAD_PLUGINS are environment variables that can be used to control what
 * plugins are loaded. If the variable is set, it is interpreted as a list of paths separated
 * by colon (:), where each element can be a directory or library path.
 * If not set, the "default" path is scanned, which is a subdirectory called plugin under the
 * directory where the DALI library is installed.
 */
DLL_PUBLIC void daliLoadDefaultPlugins();

/**
 * @brief Returns the named reader metadata
 *  @param reader_name Name of the reader to query
 *  @param meta Pointer to metadata to be filled by the function
 */
DLL_PUBLIC void daliGetReaderMetadata(daliPipelineHandle *pipe_handle, const char *reader_name,
                                      daliReaderMetadata* meta);

/**
 * @brief Returns the backend of the operator with a given \p operator_name
 * @param operator_name Name of the operator to query
 */
DLL_PUBLIC dali_backend_t daliGetOperatorBackend(daliPipelineHandle *pipe_handle,
                                                 const char *operator_name);

/**
 * @brief Obtains the executor statistics
 *  @param operator_meta Pointer to the memory allocated by the function with operator_meta_num
 *                       number of metadata entries. To free returned metadata use
 *                       `daliFreeExecutorMetadata` function
 *  @param operator_meta_num Pointer to the variable which will tell how many meta entries
 *                           (operators) have been files
 */
DLL_PUBLIC void daliGetExecutorMetadata(daliPipelineHandle *pipe_handle,
                                        daliExecutorMetadata **operator_meta,
                                        size_t *operator_meta_num);

/**
 * @brief Frees executor metadata obtained from daliGetExecutorMetadata
 *  @param operator_meta Pointer to the memory with metadata allocated by the
 *                       `daliGetExecutorMetadata`
 *  @param operator_meta_num Number of metadata entries provided by `daliGetExecutorMetadata`
 */
DLL_PUBLIC void daliFreeExecutorMetadata(daliExecutorMetadata *operator_meta,
                                         size_t operator_meta_num);

/**
 * @brief Frees unused memory from memory pools.
 *
 * The function frees memory from all devices and host pinned memory.
 * Memory blocks that are still (even partially) used are not freed.
 */
DLL_PUBLIC void daliReleaseUnusedMemory();

/**
 * @brief Preallocates device memory
 *
 * The function ensures that after the call, the amount of memory given in `bytes` can be
 * allocated from the pool (without further requests to the OS).
 *
 * The function works by allocating and then freeing the requested number of bytes.
 * Any outstanding allocations are not taken into account - that is, the peak amount
 * of memory allocated will be the sum of pre-existing allocation and the amount given
 * in `bytes`.
 *
 * @param device_id The ordinal number of the device to allocate the memory on. If negative,
 *                  the current device as indicated by cudaGetDevice is used.
 *
 * @return Zero, if the allocation was successful, otherwise nonzero
 */
DLL_PUBLIC int daliPreallocateDeviceMemory(size_t bytes, int device_id);

/**
 * @brief Preallocates host pinned memory
 *
 * The function ensures that after the call, the amount of memory given in `bytes` can be
 * allocated from the pool (without further requests to the OS).
 *
 * The function works by allocating and then freeing the requested number of bytes.
 * Any outstanding allocations are not taken into account - that is, the peak amount
 * of memory allocated will be the sum of pre-existing allocation and the amount given
 * in `bytes`.
 *
 * @return Zero, if the allocation was successful, otherwise nonzero
 */
DLL_PUBLIC int daliPreallocatePinnedMemory(size_t bytes);

/** @brief Returns serialized pipeline checkpoint
 *
 * Saves pipeline state together with provided external context.
 *
 * @param pipe_handle Pointer to pipeline handle.
 *
 * @param external_context External context to include in the checkpoint.
 *
 * @param checkpoint Output pointer to which checkpoint data should be saved.
 *                   The buffer is allocated with daliAlloc, freeing it is caller's responsibility.
 *
 * @param n Output argument for checkpoint size in bytes.
*/
DLL_PUBLIC void daliGetSerializedCheckpoint(
  daliPipelineHandle *pipe_handle,
  const daliExternalContextCheckpoint *external_context,
  char **checkpoint, size_t *n);

/** @brief Restores pipeline state from serialized checkpoint
 *
 * Should be called before running the pipeline.
 * The pipeline needs to have checkpointing enabled.
 *
 * @param pipe_handle Pointer to pipeline handle.
 *
 * @param checkpoint Serialized checkpoint to restore from.
 *
 * @param n Size of the checkpoint, in bytes.
 *
 * @param external_context Output buffer to which checkpoint's external context will be saved.
 *                         Populated fields of the external context can be later freed with
 *                         daliDestroyExternalContextCheckpoint. Ignored if null.
*/
DLL_PUBLIC void daliRestoreFromSerializedCheckpoint(
  daliPipelineHandle *pipe_handle,
  const char *checkpoint, size_t n,
  daliExternalContextCheckpoint *external_context);

/** @brief Frees all allocated fields of daliExternalContextCheckpoint
 *
 * @param external_context External context to destroy.
*/
DLL_PUBLIC void daliDestroyExternalContextCheckpoint(
  daliExternalContextCheckpoint *external_context);

/** @brief Allocate memory.
 *
 * @param n Size, in bytes.
 *
 * @return Pointer to allocated memory or NULL on failure.
*/
DLL_PUBLIC void *daliAlloc(size_t n);

/** @brief Free memory allocated with daliAlloc.
 *
 * @param ptr Pointer to the memory buffer.
*/
DLL_PUBLIC void daliFree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif  // DALI_C_API_H_
