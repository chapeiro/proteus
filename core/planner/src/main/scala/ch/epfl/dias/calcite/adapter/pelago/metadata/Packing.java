/*
    Proteus -- High-performance query processing on heterogeneous hardware.

                            Copyright (c) 2017
        Data Intensive Applications and Systems Laboratory (DIAS)
                École Polytechnique Fédérale de Lausanne

                            All Rights Reserved.

    Permission to use, copy, modify and distribute this software and
    its documentation is hereby granted, provided that both the
    copyright notice and this permission notice appear in all copies of
    the software, derivative works or modified versions, and any
    portions thereof, and that both notices appear in supporting
    documentation.

    This code is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
    DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
    RESULTING FROM THE USE OF THIS SOFTWARE.
 */

package ch.epfl.dias.calcite.adapter.pelago.metadata;

import org.apache.calcite.linq4j.tree.Types;
import org.apache.calcite.rel.RelNode;
import org.apache.calcite.rel.metadata.Metadata;
import org.apache.calcite.rel.metadata.MetadataDef;
import org.apache.calcite.rel.metadata.MetadataHandler;
import org.apache.calcite.rel.metadata.RelMetadataQuery;

import ch.epfl.dias.calcite.adapter.pelago.traits.RelPacking;

import java.lang.reflect.Method;

public interface Packing extends Metadata {
  Method method = Types.lookupMethod(Packing.class, "packing");
  MetadataDef<Packing> DEF = MetadataDef.of(Packing.class, Packing.Handler.class, method);

  /**
   * Determines how the rows are distributed.
   */
  RelPacking packing();

  /**
   * Handler API.
   */
  interface Handler extends MetadataHandler<Packing> {
    RelPacking packing(RelNode r, RelMetadataQuery mq);
  }
}
