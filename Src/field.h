/**
 * Copyright 2019 United Kingdom Research and Innovation
 *
 * Authors: See AUTHORS
 *
 * Contact: [jianping.meng@stfc.ac.uk and/or jpmeng@gmail.com]
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice
 *    this list of conditions and the following disclaimer in the documentation
 *    and or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * ANDANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*! @brief   Implementing functions related to the flow field
 * @author  Jianping Meng
 * @details Implementing functions related to create the flow
 * field (allocate memory), set up the geometry and the boundary
 * property, and deallocate the memory.
 */
#ifndef FIELD_H
#define FIELD_H
#include <list>
#include <map>
#include <string>
#include <vector>
#include "block.h"
#include "ops_lib_core.h"
#ifdef OPS_MPI
#include "ops_mpi_core.h"
#endif
#include "type.h"
template <typename T>
class Field {
   private:
    std::map<SizeType, ops_dat> data;
    std::map<SizeType, const Block*> dataBlock;
    std::string name;
    int dim{1};
    SizeType haloDepth{1};
#ifdef OPS_3D
    SizeType spaceDim{3};
#endif
#ifdef OPS_2D
    SizeType spaceDim{2};
#endif
    std::string type;

   public:
    Field(const std::string& varName, const int dataDim = 1,
          const int halo = 1);
    Field(const char* varName, const int dataDim = 1,
          const int halo = 1);
    void CreateFieldFromScratch(const BlockGroup& blocks);
    void CreateFieldFromScratch(const Block& block);
    void CreateFieldFromFile(const std::string& fileName, const Block& block);
    void CreateFieldFromFile(const std::string& caseName, const Block& block,
                             const SizeType timeStep);
    void CreateFieldFromFile(const std::string& caseName,
                             const BlockGroup& blocks,
                             const SizeType timeStep);
    void SetDataDim(const int dataDim) { dim = dataDim; };
    void SetDataHalo(const int halo) { haloDepth = halo; };
    void WriteToHDF5(const std::string& caseName, const SizeType timeStep) const;
    int HaloDepth() const { return haloDepth; };
    int DataDim() const { return dim; };
    ~Field(){};
    ops_dat at(SizeType blockIdx) { return data.at(blockIdx); };
    ops_dat operator[](SizeType blockIdx) { return this->at(blockIdx); };
};

template <typename T>
Field<T>::Field(const std::string& varName, const int dataDim,
                const int halo) {
    name = varName;
    dim = dataDim;
    haloDepth = halo;
    std::string name{typeid(T).name()};
    if (name == "i") {
        type = "int";
    }
    if (name == "f") {
        type = "float";
    }
    if (name == "d") {
        type = "double";
    }
}
template <typename T>
Field<T>::Field(const char* varName, const int dataDim,
                const int halo) {
    name = std::string{varName};
    dim = dataDim;
    haloDepth = halo;
    std::string name{typeid(T).name()};
    if (name == "i") {
        type = "int";
    }
    if (name == "f") {
        type = "float";
    }
    if (name == "d") {
        type = "double";
    }
}

template <typename T>
void Field<T>::CreateFieldFromScratch(const Block& block) {
    T* temp = NULL;
    int* d_p = new int[spaceDim];
    int* d_m = new int[spaceDim];
    int* base = new int[spaceDim];
    for (int cordIdx = 0; cordIdx < spaceDim; cordIdx++) {
        d_p[cordIdx] = haloDepth;
        d_m[cordIdx] = -haloDepth;
        base[cordIdx] = 0;
    }
    const SizeType blockId{block.ID()};
    std::string dataName{name + "_" + block.Name()};
    std::vector<int> size{block.Size()};
    ops_dat localDat =
        ops_decl_dat((ops_block)block.Get(), dim, size.data(), base,
                     d_m, d_p, temp, type.c_str(), dataName.c_str());
    data.emplace(blockId, localDat);
    dataBlock.emplace(blockId, &block);
    delete[] d_p;
    delete[] d_m;
    delete[] base;
}

template <typename T>
void Field<T>::CreateFieldFromScratch(const BlockGroup& blocks) {
    for (const auto& idBlock : blocks) {
        const Block& block{idBlock.second};
        CreateFieldFromScratch(block);
    }
}

template <typename T>
void Field<T>::CreateFieldFromFile(const std::string& fileName,
                                   const Block& block) {
    std::string dataName{name + "_" + block.Name()};
    ops_dat localDat = ops_decl_dat_hdf5(block.Get(), dim, type.c_str(),
                                         dataName.c_str(), fileName.c_str());
    data.emplace(block.ID(), localDat);
    dataBlock.emplace(block.ID(), &block);
}

template <typename T>
void Field<T>::CreateFieldFromFile(const std::string& caseName,
                                   const Block& block,
                                   const SizeType timeStep) {
    std::string fileName{caseName + "_" + block.Name() + "_" +
                         std::to_string(timeStep)};
    CreateFieldFromFile(fileName, block);
}

template <typename T>
void Field<T>::CreateFieldFromFile(const std::string& caseName,
                                   const BlockGroup& blocks,
                                   const SizeType timeStep) {
    for (const auto& idBlock : blocks) {
        const Block& block{idBlock.second};
        CreateFieldFromFile(caseName, block, timeStep);
    }
}
template <typename T>
void Field<T>::WriteToHDF5(const std::string& caseName,
                           const SizeType timeStep) const {
    for (const auto& idData : data) {
        const SizeType blockId{idData.first};
        const Block* block{dataBlock.at(blockId)};
        std::string fileName = caseName + "_" + block->Name() + "_" +
                               std::to_string(timeStep) + ".h5";
        ops_fetch_block_hdf5_file(block->Get(), fileName.c_str());
        ops_fetch_dat_hdf5_file(idData.second, fileName.c_str());
    }
}
typedef Field<Real> RealField;
typedef Field<int> IntField;
#endif