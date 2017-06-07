#include <bitset>
#include <time.h>
#include <sstream>
#include <iostream>

#include "hll.hpp"
//#include "hll_aggregate_function.hpp"
#include "hll_vertica.hpp"

class HllDistinctCount : public AggregateFunction
{

  vint hllLeadingBits;
  uint64_t serializedSynopsisSize, deserializedSynopsisSize;
  Format format;
  uint8_t* synopsis1, *synopsis2;

public:

  virtual void setup(ServerInterface& srvInterface, const SizedColumnTypes& argTypes) {
    this -> hllLeadingBits = readSubStreamBits(srvInterface);
    this -> format = readSerializationFormat(srvInterface);
    this -> deserializedSynopsisSize =  Hll<uint64_t>::getDeserializedSynopsisSize(hllLeadingBits);
    this -> serializedSynopsisSize = Hll<uint64_t>::getSerializedSynopsisSize(format, hllLeadingBits);

    synopsis1 = vt_allocArray(srvInterface.allocator, uint8_t, deserializedSynopsisSize);
    synopsis2 = vt_allocArray(srvInterface.allocator, uint8_t, deserializedSynopsisSize);
  }

  virtual void initAggregate(ServerInterface &srvInterface, IntermediateAggs &aggs)
  {

    HLL initialHll(hllLeadingBits, synopsis1);
    try {
      initialHll.serialize(aggs.getStringRef(0).data(), format);
    } catch(SerializationError& e) {
      vt_report_error(0, e.what());
    }
  }

  void aggregate(ServerInterface &srvInterface,
                 BlockReader &argReader,
                 IntermediateAggs &aggs)
  {
    HLL outputHll(hllLeadingBits, synopsis1);
    try {
      outputHll.deserialize(aggs.getStringRef(0).data(), format);
      do {
        HLL currentSynopsis(hllLeadingBits, synopsis2);
        currentSynopsis.deserialize(argReader.getStringRef(0).data(), format);
        outputHll.add(currentSynopsis);
      } while (argReader.next());
      outputHll.serialize(aggs.getStringRef(0).data(), format);
    } catch(SerializationError& e) {
      vt_report_error(0, e.what());
    }

  }

  virtual void combine(ServerInterface &srvInterface,
                       IntermediateAggs &aggs,
                       MultipleIntermediateAggs &aggsOther)
  {
    HLL outputHll(hllLeadingBits, synopsis1);
    try {
      outputHll.deserialize(aggs.getStringRef(0).data(), format);
      do {
        HLL currentSynopsis(hllLeadingBits, synopsis2);
        currentSynopsis.deserialize(aggsOther.getStringRef(0).data(), format);
        outputHll.add(currentSynopsis);
      } while (aggsOther.next());
      outputHll.serialize(aggs.getStringRef(0).data(), format);
    } catch(SerializationError& e) {
      vt_report_error(0, e.what());
    }
  }

  virtual void terminate(ServerInterface &srvInterface,
                         BlockWriter &resWriter,
                         IntermediateAggs &aggs)
  {
    HLL finalHll(hllLeadingBits, synopsis1);
    try {
      finalHll.deserialize(aggs.getStringRef(0).data(), format);
    } catch(SerializationError& e) {
      vt_report_error(0, e.what());
    }
    resWriter.setInt(finalHll.approximateCountDistinct());
  }

  InlineAggregate()
};


class HllDistinctCountFactory : public AggregateFunctionFactory
{
  virtual void getIntermediateTypes(ServerInterface &srvInterface,
                                    const SizedColumnTypes &inputTypes,
                                    SizedColumnTypes &intermediateTypeMetaData)
  {

    Format format = readSerializationFormat(srvInterface);
    uint8_t precision = readSubStreamBits(srvInterface);
    intermediateTypeMetaData.addVarbinary(Hll<uint64_t>::getSerializedSynopsisSize(format, precision));
  }


  virtual void getPrototype(ServerInterface &srvInterface,
                            ColumnTypes &argTypes,
                            ColumnTypes &returnType)
  {
    argTypes.addVarbinary();
    returnType.addInt();
  }

  virtual void getReturnType(ServerInterface &srvInterface,
                             const SizedColumnTypes &inputTypes,
                             SizedColumnTypes &outputTypes)
  {
    outputTypes.addInt();
  }

  virtual AggregateFunction *createAggregateFunction(ServerInterface &srvInterface)
  {
    return vt_createFuncObject<HllDistinctCount>(srvInterface.allocator);
  }

  virtual void getParameterType(ServerInterface &srvInterface,
                                SizedColumnTypes &parameterTypes)
  {
    parameterTypes.addInt("_minimizeCallCount");

    SizedColumnTypes::Properties props;
    props.required = false;
    props.canBeNull = false;
    props.comment = "Precision bits";
    parameterTypes.addInt(HLL_ARRAY_SIZE_PARAMETER_NAME, props);

    props.comment = "Serialization/deserialization bits per bucket";
    parameterTypes.addInt(HLL_BITS_PER_BUCKET_PARAMETER_NAME, props);
  }

};

RegisterFactory(HllDistinctCountFactory);
RegisterLibrary("Criteo", // author
                "", // lib_build_tag
                "0.5", // lib_version
                "7.2.1", // lib_sdk_version
                "https://github.com/criteo/vertica-hyperloglog", // URL
                "HyperLogLog implementation as User Defined Aggregate Functions", // description
                "", // licenses required
                "" // signature
                );
