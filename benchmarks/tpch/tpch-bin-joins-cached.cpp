/*
    RAW -- High-performance querying over raw, never-seen-before data.

                            Copyright (c) 2014
        Data Intensive Applications and Systems Labaratory (DIAS)
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

#include "common/common.hpp"
#include "common/tpch-config.hpp"
#include "expressions/binary-operators.hpp"
#include "expressions/expressions-hasher.hpp"
#include "expressions/expressions.hpp"
#include "operators/join.hpp"
#include "operators/nest-opt.hpp"
#include "operators/nest.hpp"
#include "operators/outer-unnest.hpp"
#include "operators/print.hpp"
#include "operators/radix-join.hpp"
#include "operators/radix-nest.hpp"
#include "operators/reduce-nopred.hpp"
#include "operators/reduce-opt.hpp"
#include "operators/reduce.hpp"
#include "operators/root.hpp"
#include "operators/scan.hpp"
#include "operators/select.hpp"
#include "operators/unnest.hpp"
#include "plugins/binary-col-plugin.hpp"
#include "plugins/binary-row-plugin.hpp"
#include "plugins/csv-plugin-pm.hpp"
#include "plugins/csv-plugin.hpp"
#include "plugins/json-jsmn-plugin.hpp"
#include "plugins/json-plugin.hpp"
#include "util/raw-caching.hpp"
#include "util/raw-context.hpp"
#include "util/raw-functions.hpp"
#include "values/expressionTypes.hpp"

void tpchSchema(map<string, dataset> &datasetCatalog) {
  tpchSchemaBin(datasetCatalog);
}

// Not to be used for timing - just to materialize columns
void tpchJoinWarmupKeys(map<string, dataset> datasetCatalog);
void tpchJoinWarmupAll(map<string, dataset> datasetCatalog);
/*
   SELECT COUNT(*)
   FROM orders
   INNER JOIN lineitem ON (o_orderkey = l_orderkey)
   AND l_orderkey < [X]
 */
void tpchJoin1a(map<string, dataset> datasetCatalog, int predicate);
/*
   SELECT COUNT(*)
   FROM orders
   INNER JOIN lineitem ON (o_orderkey = l_orderkey)
   AND o_orderkey < [X]
 */
void tpchJoin1b(map<string, dataset> datasetCatalog, int predicate);

/*
   SELECT MAX(o_orderkey)
   FROM orders
   INNER JOIN lineitem ON (o_orderkey = l_orderkey)
   AND l_orderkey < [X]
 */
void tpchJoin2a(map<string, dataset> datasetCatalog, int predicate);

/*
   SELECT MAX(l_orderkey)
   FROM orders
   INNER JOIN lineitem ON (o_orderkey = l_orderkey)
   AND l_orderkey < [X]
 */
void tpchJoin2b(map<string, dataset> datasetCatalog, int predicate);

/*
   SELECT MAX(o_orderkey) , MAX(o_totalprice)
   FROM orders
   INNER JOIN lineitem ON (o_orderkey = l_orderkey)
   AND l_orderkey < [X]
 */
void tpchJoin3(map<string, dataset> datasetCatalog, int predicate);

/*
   SELECT MAX(l_orderkey) , MAX(l_extendedprice)
   FROM orders
   INNER JOIN lineitem ON (o_orderkey = l_orderkey)
   AND l_orderkey < [X]
 */
void tpchJoin4(map<string, dataset> datasetCatalog, int predicate);

// int main()    {
//
//    map<string,dataset> datasetCatalog;
//    tpchSchema(datasetCatalog);
//
//    int runs = 5;
//    int selectivityShifts = 10;
//    int predicateMax = O_ORDERKEY_MAX;
//
//    cout << "[tpch-bin-joins: ] Warmup" << endl;
//    cout << "-> tpchJoinWarmupKeys" << endl;
//    tpchJoinWarmupKeys(datasetCatalog);
//    cout << "[tpch-bin-joins: ] End of Warmup" << endl;
//    for (int i = 0; i < runs; i++) {
//        cout << "[tpch-bin-joins-keysAsFields: ] Run " << i + 1 << endl;
//        for (int i = 1; i <= selectivityShifts; i++) {
//            double ratio = (i / (double) 10);
//            double percentage = ratio * 100;
//
//            int predicateVal = (int) ceil(predicateMax * ratio);
//            cout << "SELECTIVITY FOR key < " << predicateVal << ": "
//                    << percentage << "%" << endl;
//            tpchJoin1a(datasetCatalog, predicateVal);
//
//            tpchJoin1b(datasetCatalog, predicateVal);
//
//            tpchJoin2a(datasetCatalog, predicateVal);
//
//            tpchJoin2b(datasetCatalog, predicateVal);
//
//        }
//    }
//
//    cout << "[tpch-bin-joins: ] Clean internal caches & Warmup" << endl;
//    cout << "-> tpchJoinWarmupAll" << endl;
//    /* Clean */
//    RawCatalog& rawCatalog = RawCatalog::getInstance();
//    rawCatalog.clear();
//    CachingService& cache = CachingService::getInstance();
//    cache.clear();
//    tpchJoinWarmupAll(datasetCatalog);
//    cout << "[tpch-bin-joins: ] End of Clean internal caches & Warmup" <<
//    endl; for (int i = 0; i < runs; i++) {
//        cout << "[tpch-bin-joins-moreFields: ] Run " << i + 1 << endl;
//        for (int i = 1; i <= selectivityShifts; i++) {
//            double ratio = (i / (double) 10);
//            double percentage = ratio * 100;
//
//            int predicateVal = (int) ceil(predicateMax * ratio);
//            cout << "SELECTIVITY FOR key < " << predicateVal << ": "
//                    << percentage << "%" << endl;
//            tpchJoin3(datasetCatalog, predicateVal);
//            tpchJoin4(datasetCatalog, predicateVal);
//
//        }
//    }
//
//}

/* Complementary run to fix typo (predicateMax) */
// int main()    {
//
//    map<string,dataset> datasetCatalog;
//    tpchSchema(datasetCatalog);
//
//    int runs = 5;
//    int selectivityShifts = 10;
//    int predicateMax = O_ORDERKEY_MAX;
//
//    cout << "[tpch-bin-joins: ] Clean internal caches & Warmup" << endl;
//    cout << "-> tpchJoinWarmupAll" << endl;
//    /* Clean */
//    RawCatalog& rawCatalog = RawCatalog::getInstance();
//    rawCatalog.clear();
//    CachingService& cache = CachingService::getInstance();
//    cache.clear();
//    tpchJoinWarmupAll(datasetCatalog);
//    cout << "[tpch-bin-joins: ] End of Clean internal caches & Warmup" <<
//    endl; for (int i = 0; i < runs; i++) {
//        cout << "[tpch-bin-joins-moreFields: ] Run " << i + 1 << endl;
//        for (int i = 1; i <= selectivityShifts; i++) {
//            double ratio = (i / (double) 10);
//            double percentage = ratio * 100;
//
//            int predicateVal = (int) ceil(predicateMax * ratio);
//            cout << "SELECTIVITY FOR key < " << predicateVal << ": "
//                    << percentage << "%" << endl;
//            cout << "Join3" << endl;
//            tpchJoin3(datasetCatalog, predicateVal);
//            cout << "Join4" << endl;
//            tpchJoin4(datasetCatalog, predicateVal);
//
//        }
//    }
//
//}

int main() {
  map<string, dataset> datasetCatalog;
  tpchSchema(datasetCatalog);

  int runs = 5;
  int selectivityShifts = 10;
  int predicateMax = O_ORDERKEY_MAX;

  CachingService &cache = CachingService::getInstance();
  RawCatalog &rawCatalog = RawCatalog::getInstance();
  tpchJoinWarmupAll(datasetCatalog);
  for (int i = 0; i < runs; i++) {
    cout << "[tpch-bin-joins: ] Run " << i + 1 << endl;
    for (int i = 1; i <= selectivityShifts; i++) {
      double ratio = (i / (double)10);
      double percentage = ratio * 100;

      int predicateVal = (int)ceil(predicateMax * ratio);
      cout << "SELECTIVITY FOR key < " << predicateVal << ": " << percentage
           << "%" << endl;

      cout << "1a)" << endl;
      tpchJoin1a(datasetCatalog, predicateVal);
    }
  }

  cache.clear();
  rawCatalog.clear();
  tpchJoinWarmupAll(datasetCatalog);
  for (int i = 0; i < runs; i++) {
    cout << "[tpch-bin-joins: ] Run " << i + 1 << endl;
    for (int i = 1; i <= selectivityShifts; i++) {
      double ratio = (i / (double)10);
      double percentage = ratio * 100;

      int predicateVal = (int)ceil(predicateMax * ratio);
      cout << "SELECTIVITY FOR key < " << predicateVal << ": " << percentage
           << "%" << endl;

      cout << "1b)" << endl;
      tpchJoin1b(datasetCatalog, predicateVal);
    }
  }

  cache.clear();
  rawCatalog.clear();
  tpchJoinWarmupAll(datasetCatalog);
  for (int i = 0; i < runs; i++) {
    cout << "[tpch-bin-joins: ] Run " << i + 1 << endl;
    for (int i = 1; i <= selectivityShifts; i++) {
      double ratio = (i / (double)10);
      double percentage = ratio * 100;

      int predicateVal = (int)ceil(predicateMax * ratio);
      cout << "SELECTIVITY FOR key < " << predicateVal << ": " << percentage
           << "%" << endl;

      cout << "2a)" << endl;
      tpchJoin2a(datasetCatalog, predicateVal);
    }
  }

  cache.clear();
  rawCatalog.clear();
  tpchJoinWarmupAll(datasetCatalog);
  for (int i = 0; i < runs; i++) {
    cout << "[tpch-bin-joins: ] Run " << i + 1 << endl;
    for (int i = 1; i <= selectivityShifts; i++) {
      double ratio = (i / (double)10);
      double percentage = ratio * 100;

      int predicateVal = (int)ceil(predicateMax * ratio);
      cout << "SELECTIVITY FOR key < " << predicateVal << ": " << percentage
           << "%" << endl;

      cout << "2b)" << endl;
      tpchJoin2b(datasetCatalog, predicateVal);
    }
  }

  cache.clear();
  rawCatalog.clear();
  tpchJoinWarmupAll(datasetCatalog);
  for (int i = 0; i < runs; i++) {
    cout << "[tpch-bin-joins: ] Run " << i + 1 << endl;
    for (int i = 1; i <= selectivityShifts; i++) {
      double ratio = (i / (double)10);
      double percentage = ratio * 100;

      int predicateVal = (int)ceil(predicateMax * ratio);
      cout << "SELECTIVITY FOR key < " << predicateVal << ": " << percentage
           << "%" << endl;

      cout << "3)" << endl;
      tpchJoin3(datasetCatalog, predicateVal);
    }
  }

  cache.clear();
  rawCatalog.clear();
  tpchJoinWarmupAll(datasetCatalog);
  for (int i = 0; i < runs; i++) {
    cout << "[tpch-bin-joins: ] Run " << i + 1 << endl;
    for (int i = 1; i <= selectivityShifts; i++) {
      double ratio = (i / (double)10);
      double percentage = ratio * 100;

      int predicateVal = (int)ceil(predicateMax * ratio);
      cout << "SELECTIVITY FOR key < " << predicateVal << ": " << percentage
           << "%" << endl;

      cout << "4)" << endl;
      tpchJoin4(datasetCatalog, predicateVal);
    }
  }
}

void tpchJoinWarmupKeys(map<string, dataset> datasetCatalog) {
  RawContext &ctx = *prepareContext("tpch-csv-join1a");
  RawCatalog &rawCatalog = RawCatalog::getInstance();

  string nameLineitem = string("lineitem");
  dataset lineitem = datasetCatalog[nameLineitem];
  string nameOrders = string("orders");
  dataset orders = datasetCatalog[nameOrders];
  map<string, RecordAttribute *> argsLineitem = lineitem.recType.getArgsMap();
  map<string, RecordAttribute *> argsOrder = orders.recType.getArgsMap();

  /**
   * SCAN 1: Orders
   */
  string ordersNamePrefix = orders.path;
  RecordType recOrders = orders.recType;
  int policy = 5;
  int lineHint = orders.linehint;
  char delimInner = '|';
  vector<RecordAttribute *> orderProjections;
  RecordAttribute *o_orderkey = argsOrder["o_orderkey"];
  orderProjections.push_back(o_orderkey);

  BinaryColPlugin *pgOrders =
      new BinaryColPlugin(&ctx, ordersNamePrefix, recOrders, orderProjections);
  rawCatalog.registerPlugin(ordersNamePrefix, pgOrders);
  Scan *scanOrders = new Scan(&ctx, *pgOrders);

  /**
   * SCAN 2: Lineitem
   */
  string lineitemNamePrefix = lineitem.path;
  RecordType recLineitem = lineitem.recType;
  policy = 5;
  lineHint = lineitem.linehint;
  delimInner = '|';
  vector<RecordAttribute *> lineitemProjections;
  RecordAttribute *l_orderkey = argsLineitem["l_orderkey"];
  lineitemProjections.push_back(l_orderkey);

  BinaryColPlugin *pgLineitem = new BinaryColPlugin(
      &ctx, lineitemNamePrefix, recLineitem, lineitemProjections);
  rawCatalog.registerPlugin(lineitemNamePrefix, pgLineitem);
  Scan *scanLineitem = new Scan(&ctx, *pgLineitem);

  list<RecordAttribute> argProjectionsRight;
  argProjectionsRight.push_back(*l_orderkey);
  expressions::Expression *rightArg =
      new expressions::InputArgument(&recLineitem, 1, argProjectionsRight);

  /**
   * JOIN
   */
  /* join key - orders */
  list<RecordAttribute> argProjectionsLeft;
  argProjectionsLeft.push_back(*o_orderkey);
  expressions::Expression *leftArg =
      new expressions::InputArgument(&recOrders, 0, argProjectionsLeft);
  expressions::Expression *leftPred = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);

  /* join key - lineitem */
  expressions::Expression *rightPred = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);

  /* join pred. */
  expressions::BinaryExpression *joinPred =
      new expressions::EqExpression(leftPred, rightPred);

  /* left materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsLeft;
  fieldsLeft.push_back(o_orderkey);
  vector<materialization_mode> outputModesLeft;
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);

  /* explicit mention to left OID */
  RecordAttribute *projTupleL =
      new RecordAttribute(ordersNamePrefix, activeLoop, pgOrders->getOIDType());
  vector<RecordAttribute *> OIDLeft;
  OIDLeft.push_back(projTupleL);
  expressions::Expression *exprLeftOID = new expressions::RecordProjection(
      pgOrders->getOIDType(), leftArg, *projTupleL);
  expressions::Expression *exprOrderkey = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);
  vector<expression_t> expressionsLeft;
  expressionsLeft.push_back(exprLeftOID);
  expressionsLeft.push_back(exprOrderkey);

  Materializer *matLeft =
      new Materializer(fieldsLeft, expressionsLeft, OIDLeft, outputModesLeft);

  /* right materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsRight;
  fieldsRight.push_back(l_orderkey);
  vector<materialization_mode> outputModesRight;
  outputModesRight.insert(outputModesRight.begin(), EAGER);

  /* explicit mention to right OID */
  RecordAttribute *projTupleR = new RecordAttribute(
      lineitemNamePrefix, activeLoop, pgLineitem->getOIDType());
  vector<RecordAttribute *> OIDRight;
  OIDRight.push_back(projTupleR);
  expressions::Expression *exprRightOID = new expressions::RecordProjection(
      pgLineitem->getOIDType(), rightArg, *projTupleR);
  expressions::Expression *exprL_Orderkey = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);
  vector<expression_t> expressionsRight;
  expressionsRight.push_back(exprRightOID);
  expressionsRight.push_back(exprL_Orderkey);

  Materializer *matRight = new Materializer(fieldsRight, expressionsRight,
                                            OIDRight, outputModesRight);

  char joinLabel[] = "radixJoin";
  RadixJoin *join = new RadixJoin(*joinPred, scanOrders, scanLineitem, &ctx,
                                  joinLabel, *matLeft, *matRight);
  scanOrders->setParent(join);
  scanLineitem->setParent(join);

  /**
   * REDUCE
   * COUNT(*)
   */
  /* Output: */
  expressions::Expression *outputExpr = new expressions::IntConstant(1);
  ReduceNoPred *reduce = new ReduceNoPred(SUM, outputExpr, join, &ctx);
  join->setParent(reduce);

  // Run function
  struct timespec t0, t1;
  clock_gettime(CLOCK_REALTIME, &t0);
  reduce->produce();
  ctx.prepareFunction(ctx.getGlobalFunction());
  clock_gettime(CLOCK_REALTIME, &t1);
  printf("Execution took %f seconds\n", diff(t0, t1));

  // Close all open files & clear
  pgOrders->finish();
  pgLineitem->finish();
  rawCatalog.clear();
}

void tpchJoin1a(map<string, dataset> datasetCatalog, int predicate) {
  RawContext &ctx = *prepareContext("tpch-csv-join1a");
  RawCatalog &rawCatalog = RawCatalog::getInstance();

  string nameLineitem = string("lineitem");
  dataset lineitem = datasetCatalog[nameLineitem];
  string nameOrders = string("orders");
  dataset orders = datasetCatalog[nameOrders];
  map<string, RecordAttribute *> argsLineitem = lineitem.recType.getArgsMap();
  map<string, RecordAttribute *> argsOrder = orders.recType.getArgsMap();

  /**
   * SCAN 1: Orders
   */
  string ordersNamePrefix = orders.path;
  RecordType recOrders = orders.recType;
  int policy = 5;
  int lineHint = orders.linehint;
  char delimInner = '|';
  vector<RecordAttribute *> orderProjections;
  RecordAttribute *o_orderkey = argsOrder["o_orderkey"];
  orderProjections.push_back(o_orderkey);

  BinaryColPlugin *pgOrders =
      new BinaryColPlugin(&ctx, ordersNamePrefix, recOrders, orderProjections);
  rawCatalog.registerPlugin(ordersNamePrefix, pgOrders);
  Scan *scanOrders = new Scan(&ctx, *pgOrders);

  /**
   * SCAN 2: Lineitem
   */
  string lineitemNamePrefix = lineitem.path;
  RecordType recLineitem = lineitem.recType;
  policy = 5;
  lineHint = lineitem.linehint;
  delimInner = '|';
  vector<RecordAttribute *> lineitemProjections;
  RecordAttribute *l_orderkey = argsLineitem["l_orderkey"];
  lineitemProjections.push_back(l_orderkey);

  BinaryColPlugin *pgLineitem = new BinaryColPlugin(
      &ctx, lineitemNamePrefix, recLineitem, lineitemProjections);
  rawCatalog.registerPlugin(lineitemNamePrefix, pgLineitem);
  Scan *scanLineitem = new Scan(&ctx, *pgLineitem);

  /*
   * SELECT on LINEITEM
   */
  list<RecordAttribute> argProjectionsRight;
  argProjectionsRight.push_back(*l_orderkey);
  expressions::Expression *rightArg =
      new expressions::InputArgument(&recLineitem, 1, argProjectionsRight);
  expressions::Expression *lhs = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);
  expressions::Expression *rhs = new expressions::IntConstant(predicate);
  expressions::Expression *pred = new expressions::LtExpression(lhs, rhs);

  Select *sel = new Select(pred, scanLineitem);
  scanLineitem->setParent(sel);

  /**
   * JOIN
   */
  /* join key - orders */
  list<RecordAttribute> argProjectionsLeft;
  argProjectionsLeft.push_back(*o_orderkey);
  expressions::Expression *leftArg =
      new expressions::InputArgument(&recOrders, 0, argProjectionsLeft);
  expressions::Expression *leftPred = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);

  /* join key - lineitem */
  expressions::Expression *rightPred = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);

  /* join pred. */
  expressions::BinaryExpression *joinPred =
      new expressions::EqExpression(leftPred, rightPred);

  /* left materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsLeft;
  fieldsLeft.push_back(o_orderkey);
  vector<materialization_mode> outputModesLeft;
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);

  /* explicit mention to left OID */
  RecordAttribute *projTupleL =
      new RecordAttribute(ordersNamePrefix, activeLoop, pgOrders->getOIDType());
  vector<RecordAttribute *> OIDLeft;
  OIDLeft.push_back(projTupleL);
  expressions::Expression *exprLeftOID = new expressions::RecordProjection(
      pgOrders->getOIDType(), leftArg, *projTupleL);
  expressions::Expression *exprOrderkey = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);
  vector<expression_t> expressionsLeft;
  expressionsLeft.push_back(exprLeftOID);
  expressionsLeft.push_back(exprOrderkey);

  Materializer *matLeft =
      new Materializer(fieldsLeft, expressionsLeft, OIDLeft, outputModesLeft);

  /* right materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsRight;
  vector<materialization_mode> outputModesRight;

  /* explicit mention to right OID */
  RecordAttribute *projTupleR = new RecordAttribute(
      lineitemNamePrefix, activeLoop, pgLineitem->getOIDType());
  vector<RecordAttribute *> OIDRight;
  OIDRight.push_back(projTupleR);
  expressions::Expression *exprRightOID = new expressions::RecordProjection(
      pgLineitem->getOIDType(), rightArg, *projTupleR);
  vector<expression_t> expressionsRight;
  expressionsRight.push_back(exprRightOID);

  Materializer *matRight = new Materializer(fieldsRight, expressionsRight,
                                            OIDRight, outputModesRight);

  char joinLabel[] = "radixJoin";
  RadixJoin *join = new RadixJoin(*joinPred, scanOrders, sel, &ctx, joinLabel,
                                  *matLeft, *matRight);
  scanOrders->setParent(join);
  sel->setParent(join);

  /**
   * REDUCE
   * COUNT(*)
   */
  /* Output: */
  expressions::Expression *outputExpr = new expressions::IntConstant(1);
  ReduceNoPred *reduce = new ReduceNoPred(SUM, outputExpr, join, &ctx);
  join->setParent(reduce);

  // Run function
  struct timespec t0, t1;
  clock_gettime(CLOCK_REALTIME, &t0);
  reduce->produce();
  ctx.prepareFunction(ctx.getGlobalFunction());
  clock_gettime(CLOCK_REALTIME, &t1);
  printf("Execution took %f seconds\n", diff(t0, t1));

  // Close all open files & clear
  pgOrders->finish();
  pgLineitem->finish();
  rawCatalog.clear();
}

void tpchJoin1b(map<string, dataset> datasetCatalog, int predicate) {
  RawContext &ctx = *prepareContext("tpch-csv-join1b");
  RawCatalog &rawCatalog = RawCatalog::getInstance();

  string nameLineitem = string("lineitem");
  dataset lineitem = datasetCatalog[nameLineitem];
  string nameOrders = string("orders");
  dataset orders = datasetCatalog[nameOrders];
  map<string, RecordAttribute *> argsLineitem = lineitem.recType.getArgsMap();
  map<string, RecordAttribute *> argsOrder = orders.recType.getArgsMap();

  /**
   * SCAN 1: Orders
   */
  string ordersNamePrefix = orders.path;
  RecordType recOrders = orders.recType;
  int policy = 5;
  int lineHint = orders.linehint;
  char delimInner = '|';
  vector<RecordAttribute *> orderProjections;
  RecordAttribute *o_orderkey = argsOrder["o_orderkey"];
  orderProjections.push_back(o_orderkey);

  BinaryColPlugin *pgOrders =
      new BinaryColPlugin(&ctx, ordersNamePrefix, recOrders, orderProjections);
  rawCatalog.registerPlugin(ordersNamePrefix, pgOrders);
  Scan *scanOrders = new Scan(&ctx, *pgOrders);

  /**
   * SCAN 2: Lineitem
   */
  string lineitemNamePrefix = lineitem.path;
  RecordType recLineitem = lineitem.recType;
  policy = 5;
  lineHint = lineitem.linehint;
  delimInner = '|';
  vector<RecordAttribute *> lineitemProjections;
  RecordAttribute *l_orderkey = argsLineitem["l_orderkey"];
  lineitemProjections.push_back(l_orderkey);

  BinaryColPlugin *pgLineitem = new BinaryColPlugin(
      &ctx, lineitemNamePrefix, recLineitem, lineitemProjections);
  rawCatalog.registerPlugin(lineitemNamePrefix, pgLineitem);
  Scan *scanLineitem = new Scan(&ctx, *pgLineitem);

  /*
   * SELECT on ORDERS
   */
  list<RecordAttribute> argProjectionsLeft;
  argProjectionsLeft.push_back(*o_orderkey);
  expressions::Expression *leftArg =
      new expressions::InputArgument(&recOrders, 0, argProjectionsLeft);
  expressions::Expression *lhs = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);
  expressions::Expression *rhs = new expressions::IntConstant(predicate);
  expressions::Expression *pred = new expressions::LtExpression(lhs, rhs);

  Select *sel = new Select(pred, scanOrders);
  scanOrders->setParent(sel);

  /**
   * JOIN
   */
  /* join key - orders */
  expressions::Expression *leftPred = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);

  /* join key - lineitem */
  list<RecordAttribute> argProjectionsRight;
  argProjectionsRight.push_back(*l_orderkey);
  expressions::Expression *rightArg =
      new expressions::InputArgument(&recLineitem, 1, argProjectionsRight);
  expressions::Expression *rightPred = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);

  /* join pred. */
  expressions::BinaryExpression *joinPred =
      new expressions::EqExpression(leftPred, rightPred);

  /* left materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsLeft;
  vector<materialization_mode> outputModesLeft;

  /* explicit mention to left OID */
  RecordAttribute *projTupleL =
      new RecordAttribute(ordersNamePrefix, activeLoop, pgOrders->getOIDType());
  vector<RecordAttribute *> OIDLeft;
  OIDLeft.push_back(projTupleL);
  expressions::Expression *exprLeftOID = new expressions::RecordProjection(
      pgOrders->getOIDType(), leftArg, *projTupleL);
  vector<expression_t> expressionsLeft;
  expressionsLeft.push_back(exprLeftOID);

  Materializer *matLeft =
      new Materializer(fieldsLeft, expressionsLeft, OIDLeft, outputModesLeft);

  /* right materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsRight;
  fieldsRight.push_back(l_orderkey);
  vector<materialization_mode> outputModesRight;
  outputModesRight.insert(outputModesRight.begin(), EAGER);

  /* explicit mention to right OID */
  RecordAttribute *projTupleR = new RecordAttribute(
      lineitemNamePrefix, activeLoop, pgLineitem->getOIDType());
  vector<RecordAttribute *> OIDRight;
  OIDRight.push_back(projTupleR);
  expressions::Expression *exprRightOID = new expressions::RecordProjection(
      pgLineitem->getOIDType(), rightArg, *projTupleR);
  expressions::Expression *exprOrderkey = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);
  vector<expression_t> expressionsRight;
  expressionsRight.push_back(exprRightOID);
  expressionsRight.push_back(exprOrderkey);

  Materializer *matRight = new Materializer(fieldsRight, expressionsRight,
                                            OIDRight, outputModesRight);

  char joinLabel[] = "radixJoin";
  RadixJoin *join = new RadixJoin(*joinPred, sel, scanLineitem, &ctx, joinLabel,
                                  *matLeft, *matRight);

  sel->setParent(join);
  scanLineitem->setParent(join);

  /**
   * REDUCE
   * COUNT(*)
   */
  /* Output: */
  expressions::Expression *outputExpr = new expressions::IntConstant(1);
  ReduceNoPred *reduce = new ReduceNoPred(SUM, outputExpr, join, &ctx);
  join->setParent(reduce);

  // Run function
  struct timespec t0, t1;
  clock_gettime(CLOCK_REALTIME, &t0);
  reduce->produce();
  ctx.prepareFunction(ctx.getGlobalFunction());
  clock_gettime(CLOCK_REALTIME, &t1);
  printf("Execution took %f seconds\n", diff(t0, t1));

  // Close all open files & clear
  pgOrders->finish();
  pgLineitem->finish();
  rawCatalog.clear();
}

void tpchJoin2a(map<string, dataset> datasetCatalog, int predicate) {
  RawContext &ctx = *prepareContext("tpch-csv-join2a");
  RawCatalog &rawCatalog = RawCatalog::getInstance();

  string nameLineitem = string("lineitem");
  dataset lineitem = datasetCatalog[nameLineitem];
  string nameOrders = string("orders");
  dataset orders = datasetCatalog[nameOrders];
  map<string, RecordAttribute *> argsLineitem = lineitem.recType.getArgsMap();
  map<string, RecordAttribute *> argsOrder = orders.recType.getArgsMap();

  /**
   * SCAN 1: Orders
   */
  string ordersNamePrefix = orders.path;
  RecordType recOrders = orders.recType;
  int policy = 5;
  int lineHint = orders.linehint;
  char delimInner = '|';
  vector<RecordAttribute *> orderProjections;
  RecordAttribute *o_orderkey = argsOrder["o_orderkey"];
  orderProjections.push_back(o_orderkey);

  BinaryColPlugin *pgOrders =
      new BinaryColPlugin(&ctx, ordersNamePrefix, recOrders, orderProjections);
  rawCatalog.registerPlugin(ordersNamePrefix, pgOrders);
  Scan *scanOrders = new Scan(&ctx, *pgOrders);

  /**
   * SCAN 2: Lineitem
   */
  string lineitemNamePrefix = lineitem.path;
  RecordType recLineitem = lineitem.recType;
  policy = 5;
  lineHint = lineitem.linehint;
  delimInner = '|';
  vector<RecordAttribute *> lineitemProjections;
  RecordAttribute *l_orderkey = argsLineitem["l_orderkey"];
  lineitemProjections.push_back(l_orderkey);

  BinaryColPlugin *pgLineitem = new BinaryColPlugin(
      &ctx, lineitemNamePrefix, recLineitem, lineitemProjections);
  rawCatalog.registerPlugin(lineitemNamePrefix, pgLineitem);
  Scan *scanLineitem = new Scan(&ctx, *pgLineitem);

  /*
   * SELECT on LINEITEM
   */
  list<RecordAttribute> argProjectionsRight;
  argProjectionsRight.push_back(*l_orderkey);
  expressions::Expression *rightArg =
      new expressions::InputArgument(&recLineitem, 1, argProjectionsRight);
  expressions::Expression *lhs = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);
  expressions::Expression *rhs = new expressions::IntConstant(predicate);
  expressions::Expression *pred = new expressions::LtExpression(lhs, rhs);

  Select *sel = new Select(pred, scanLineitem);
  scanLineitem->setParent(sel);

  /**
   * JOIN
   */
  /* join key - orders */
  list<RecordAttribute> argProjectionsLeft;
  argProjectionsLeft.push_back(*o_orderkey);
  expressions::Expression *leftArg =
      new expressions::InputArgument(&recOrders, 0, argProjectionsLeft);
  expressions::Expression *leftPred = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);

  /* join key - lineitem */
  expressions::Expression *rightPred = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);

  /* join pred. */
  expressions::BinaryExpression *joinPred =
      new expressions::EqExpression(leftPred, rightPred);

  /* left materializer - no explicit field needed - added them to help caching
   */
  vector<RecordAttribute *> fieldsLeft;
  fieldsLeft.push_back(o_orderkey);
  vector<materialization_mode> outputModesLeft;
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);

  /* explicit mention to left OID */
  RecordAttribute *projTupleL =
      new RecordAttribute(ordersNamePrefix, activeLoop, pgOrders->getOIDType());
  vector<RecordAttribute *> OIDLeft;
  OIDLeft.push_back(projTupleL);
  expressions::Expression *exprLeftOID = new expressions::RecordProjection(
      pgOrders->getOIDType(), leftArg, *projTupleL);
  expressions::Expression *exprOrderkey = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);
  vector<expression_t> expressionsLeft;
  expressionsLeft.push_back(exprLeftOID);
  expressionsLeft.push_back(exprOrderkey);

  Materializer *matLeft =
      new Materializer(fieldsLeft, expressionsLeft, OIDLeft, outputModesLeft);

  /* right materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsRight;
  vector<materialization_mode> outputModesRight;

  /* explicit mention to right OID */
  RecordAttribute *projTupleR = new RecordAttribute(
      lineitemNamePrefix, activeLoop, pgLineitem->getOIDType());
  vector<RecordAttribute *> OIDRight;
  OIDRight.push_back(projTupleR);
  expressions::Expression *exprRightOID = new expressions::RecordProjection(
      pgLineitem->getOIDType(), rightArg, *projTupleR);
  vector<expression_t> expressionsRight;
  expressionsRight.push_back(exprRightOID);

  Materializer *matRight = new Materializer(fieldsRight, expressionsRight,
                                            OIDRight, outputModesRight);

  char joinLabel[] = "radixJoin";
  RadixJoin *join = new RadixJoin(*joinPred, scanOrders, sel, &ctx, joinLabel,
                                  *matLeft, *matRight);
  scanOrders->setParent(join);
  sel->setParent(join);

  /**
   * REDUCE
   * MAX(orderkey)
   */
  /* Output: */
  expressions::Expression *outputExpr = exprOrderkey;
  ReduceNoPred *reduce = new ReduceNoPred(MAX, outputExpr, join, &ctx);
  join->setParent(reduce);

  // Run function
  struct timespec t0, t1;
  clock_gettime(CLOCK_REALTIME, &t0);
  reduce->produce();
  ctx.prepareFunction(ctx.getGlobalFunction());
  clock_gettime(CLOCK_REALTIME, &t1);
  printf("Execution took %f seconds\n", diff(t0, t1));

  // Close all open files & clear
  pgOrders->finish();
  pgLineitem->finish();
  rawCatalog.clear();
}

void tpchJoin2b(map<string, dataset> datasetCatalog, int predicate) {
  RawContext &ctx = *prepareContext("tpch-csv-join2b");
  RawCatalog &rawCatalog = RawCatalog::getInstance();

  string nameLineitem = string("lineitem");
  dataset lineitem = datasetCatalog[nameLineitem];
  string nameOrders = string("orders");
  dataset orders = datasetCatalog[nameOrders];
  map<string, RecordAttribute *> argsLineitem = lineitem.recType.getArgsMap();
  map<string, RecordAttribute *> argsOrder = orders.recType.getArgsMap();

  /**
   * SCAN 1: Orders
   */
  string ordersNamePrefix = orders.path;
  RecordType recOrders = orders.recType;
  int policy = 5;
  int lineHint = orders.linehint;
  char delimInner = '|';
  vector<RecordAttribute *> orderProjections;
  RecordAttribute *o_orderkey = argsOrder["o_orderkey"];
  orderProjections.push_back(o_orderkey);

  BinaryColPlugin *pgOrders =
      new BinaryColPlugin(&ctx, ordersNamePrefix, recOrders, orderProjections);
  rawCatalog.registerPlugin(ordersNamePrefix, pgOrders);
  Scan *scanOrders = new Scan(&ctx, *pgOrders);

  /**
   * SCAN 2: Lineitem
   */
  string lineitemNamePrefix = lineitem.path;
  RecordType recLineitem = lineitem.recType;
  policy = 5;
  lineHint = lineitem.linehint;
  delimInner = '|';
  vector<RecordAttribute *> lineitemProjections;
  RecordAttribute *l_orderkey = argsLineitem["l_orderkey"];
  lineitemProjections.push_back(l_orderkey);

  BinaryColPlugin *pgLineitem = new BinaryColPlugin(
      &ctx, lineitemNamePrefix, recLineitem, lineitemProjections);
  rawCatalog.registerPlugin(lineitemNamePrefix, pgLineitem);
  Scan *scanLineitem = new Scan(&ctx, *pgLineitem);

  //    /*
  //     * SELECT on ORDERS
  //     */
  //    list<RecordAttribute> argProjectionsLeft;
  //    argProjectionsLeft.push_back(*o_orderkey);
  //    expressions::Expression* leftArg = new expressions::InputArgument(
  //            &recOrders, 0, argProjectionsLeft);
  //    expressions::Expression *lhs = new expressions::RecordProjection(
  //            o_orderkey->getOriginalType(), leftArg, *o_orderkey);
  //    expressions::Expression* rhs = new expressions::IntConstant(predicate);
  //    expressions::Expression* pred = new expressions::LtExpression(
  //            lhs, rhs);
  //
  //    Select *sel = new Select(pred, scanOrders);
  //    scanOrders->setParent(sel);

  /*
   * SELECT on LINEITEM
   */
  list<RecordAttribute> argProjectionsRight;
  argProjectionsRight.push_back(*l_orderkey);
  expressions::Expression *rightArg =
      new expressions::InputArgument(&recLineitem, 1, argProjectionsRight);
  expressions::Expression *lhs = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);
  expressions::Expression *rhs = new expressions::IntConstant(predicate);
  expressions::Expression *pred = new expressions::LtExpression(lhs, rhs);

  Select *sel = new Select(pred, scanLineitem);
  scanLineitem->setParent(sel);

  /**
   * JOIN
   */
  /* join key - orders */
  list<RecordAttribute> argProjectionsLeft;
  argProjectionsLeft.push_back(*o_orderkey);
  expressions::Expression *leftArg =
      new expressions::InputArgument(&recOrders, 0, argProjectionsLeft);

  expressions::Expression *leftPred = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);

  /* join key - lineitem */
  //    list<RecordAttribute> argProjectionsRight;
  //    argProjectionsRight.push_back(*l_orderkey);
  //    expressions::Expression* rightArg = new expressions::InputArgument(
  //            &recLineitem, 1, argProjectionsRight);
  expressions::Expression *rightPred = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);

  /* join pred. */
  expressions::BinaryExpression *joinPred =
      new expressions::EqExpression(leftPred, rightPred);

  /* left materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsLeft;
  fieldsLeft.push_back(o_orderkey);
  vector<materialization_mode> outputModesLeft;
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);

  /* explicit mention to left OID */
  RecordAttribute *projTupleL =
      new RecordAttribute(ordersNamePrefix, activeLoop, pgOrders->getOIDType());
  vector<RecordAttribute *> OIDLeft;
  OIDLeft.push_back(projTupleL);
  expressions::Expression *exprLeftOID = new expressions::RecordProjection(
      pgOrders->getOIDType(), leftArg, *projTupleL);
  expressions::Expression *exprO_Orderkey = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);
  vector<expression_t> expressionsLeft;
  expressionsLeft.push_back(exprLeftOID);
  expressionsLeft.push_back(exprO_Orderkey);

  Materializer *matLeft =
      new Materializer(fieldsLeft, expressionsLeft, OIDLeft, outputModesLeft);

  /* right materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsRight;
  fieldsRight.push_back(l_orderkey);
  vector<materialization_mode> outputModesRight;
  outputModesRight.insert(outputModesRight.begin(), EAGER);

  /* explicit mention to right OID */
  RecordAttribute *projTupleR = new RecordAttribute(
      lineitemNamePrefix, activeLoop, pgLineitem->getOIDType());
  vector<RecordAttribute *> OIDRight;
  OIDRight.push_back(projTupleR);
  expressions::Expression *exprRightOID = new expressions::RecordProjection(
      pgLineitem->getOIDType(), rightArg, *projTupleR);
  expressions::Expression *exprOrderkey = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);

  vector<expression_t> expressionsRight;
  expressionsRight.push_back(exprRightOID);
  expressionsRight.push_back(exprOrderkey);

  Materializer *matRight = new Materializer(fieldsRight, expressionsRight,
                                            OIDRight, outputModesRight);

  char joinLabel[] = "radixJoin";
  RadixJoin *join = new RadixJoin(*joinPred, scanOrders, sel, &ctx, joinLabel,
                                  *matLeft, *matRight);

  sel->setParent(join);
  scanOrders->setParent(join);

  /**
   * REDUCE
   * MAX(orderkey)
   */
  /* Output: */
  expressions::Expression *outputExpr = exprOrderkey;
  ReduceNoPred *reduce = new ReduceNoPred(MAX, outputExpr, join, &ctx);
  join->setParent(reduce);

  // Run function
  struct timespec t0, t1;
  clock_gettime(CLOCK_REALTIME, &t0);
  reduce->produce();
  ctx.prepareFunction(ctx.getGlobalFunction());
  clock_gettime(CLOCK_REALTIME, &t1);
  printf("Execution took %f seconds\n", diff(t0, t1));

  // Close all open files & clear
  pgOrders->finish();
  pgLineitem->finish();
  rawCatalog.clear();
}

void tpchJoin3(map<string, dataset> datasetCatalog, int predicate) {
  RawContext &ctx = *prepareContext("tpch-csv-join3");
  RawCatalog &rawCatalog = RawCatalog::getInstance();

  string nameLineitem = string("lineitem");
  dataset lineitem = datasetCatalog[nameLineitem];
  string nameOrders = string("orders");
  dataset orders = datasetCatalog[nameOrders];
  map<string, RecordAttribute *> argsLineitem = lineitem.recType.getArgsMap();
  map<string, RecordAttribute *> argsOrder = orders.recType.getArgsMap();

  /**
   * SCAN 1: Orders
   */
  string ordersNamePrefix = orders.path;
  RecordType recOrders = orders.recType;
  int policy = 5;
  int lineHint = orders.linehint;
  char delimInner = '|';
  vector<RecordAttribute *> orderProjections;
  RecordAttribute *o_orderkey = argsOrder["o_orderkey"];
  RecordAttribute *o_totalprice = argsOrder["o_totalprice"];
  orderProjections.push_back(o_orderkey);
  orderProjections.push_back(o_totalprice);

  BinaryColPlugin *pgOrders =
      new BinaryColPlugin(&ctx, ordersNamePrefix, recOrders, orderProjections);
  rawCatalog.registerPlugin(ordersNamePrefix, pgOrders);
  Scan *scanOrders = new Scan(&ctx, *pgOrders);

  /**
   * SCAN 2: Lineitem
   */
  string lineitemNamePrefix = lineitem.path;
  RecordType recLineitem = lineitem.recType;
  policy = 5;
  lineHint = lineitem.linehint;
  delimInner = '|';
  vector<RecordAttribute *> lineitemProjections;
  RecordAttribute *l_orderkey = argsLineitem["l_orderkey"];
  lineitemProjections.push_back(l_orderkey);

  BinaryColPlugin *pgLineitem = new BinaryColPlugin(
      &ctx, lineitemNamePrefix, recLineitem, lineitemProjections);
  rawCatalog.registerPlugin(lineitemNamePrefix, pgLineitem);
  Scan *scanLineitem = new Scan(&ctx, *pgLineitem);

  /*
   * SELECT on LINEITEM
   */
  list<RecordAttribute> argProjectionsRight;
  argProjectionsRight.push_back(*l_orderkey);
  expressions::Expression *rightArg =
      new expressions::InputArgument(&recLineitem, 1, argProjectionsRight);
  expressions::Expression *lhs = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);
  expressions::Expression *rhs = new expressions::IntConstant(predicate);
  expressions::Expression *pred = new expressions::LtExpression(lhs, rhs);

  Select *sel = new Select(pred, scanLineitem);
  scanLineitem->setParent(sel);

  /**
   * JOIN
   */
  /* join key - orders */
  list<RecordAttribute> argProjectionsLeft;
  argProjectionsLeft.push_back(*o_orderkey);
  expressions::Expression *leftArg =
      new expressions::InputArgument(&recOrders, 0, argProjectionsLeft);
  expressions::Expression *leftPred = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);

  /* join key - lineitem */
  expressions::Expression *rightPred = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);

  /* join pred. */
  expressions::BinaryExpression *joinPred =
      new expressions::EqExpression(leftPred, rightPred);

  /* left materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsLeft;
  fieldsLeft.push_back(o_orderkey);
  fieldsLeft.push_back(o_totalprice);
  vector<materialization_mode> outputModesLeft;
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);

  /* explicit mention to left OID */
  RecordAttribute *projTupleL =
      new RecordAttribute(ordersNamePrefix, activeLoop, pgOrders->getOIDType());
  vector<RecordAttribute *> OIDLeft;
  OIDLeft.push_back(projTupleL);
  expressions::Expression *exprLeftOID = new expressions::RecordProjection(
      pgOrders->getOIDType(), leftArg, *projTupleL);
  expressions::Expression *exprOrderkey = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);
  expressions::Expression *exprTotalprice = new expressions::RecordProjection(
      o_totalprice->getOriginalType(), leftArg, *o_totalprice);
  vector<expression_t> expressionsLeft;
  expressionsLeft.push_back(exprLeftOID);
  expressionsLeft.push_back(exprOrderkey);
  expressionsLeft.push_back(exprTotalprice);

  Materializer *matLeft =
      new Materializer(fieldsLeft, expressionsLeft, OIDLeft, outputModesLeft);

  /* right materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsRight;
  vector<materialization_mode> outputModesRight;

  /* explicit mention to right OID */
  RecordAttribute *projTupleR = new RecordAttribute(
      lineitemNamePrefix, activeLoop, pgLineitem->getOIDType());
  vector<RecordAttribute *> OIDRight;
  OIDRight.push_back(projTupleR);
  expressions::Expression *exprRightOID = new expressions::RecordProjection(
      pgLineitem->getOIDType(), rightArg, *projTupleR);
  vector<expression_t> expressionsRight;
  expressionsRight.push_back(exprRightOID);

  Materializer *matRight = new Materializer(fieldsRight, expressionsRight,
                                            OIDRight, outputModesRight);

  char joinLabel[] = "radixJoin";
  RadixJoin *join = new RadixJoin(*joinPred, scanOrders, sel, &ctx, joinLabel,
                                  *matLeft, *matRight);
  scanOrders->setParent(join);
  sel->setParent(join);

  /**
   * REDUCE
   * MAX(o_orderkey), MAX(o_totalprice)
   */
  /* Output: */
  vector<Monoid> accs;
  vector<expression_t> outputExprs;
  accs.push_back(MAX);
  accs.push_back(MAX);
  outputExprs.push_back(exprOrderkey);
  outputExprs.push_back(exprTotalprice);

  expressions::Expression *outputExpr = exprOrderkey;
  opt::Reduce *reduce = new opt::Reduce(accs, outputExprs, true, join, &ctx);
  join->setParent(reduce);

  // Run function
  struct timespec t0, t1;
  clock_gettime(CLOCK_REALTIME, &t0);
  reduce->produce();
  ctx.prepareFunction(ctx.getGlobalFunction());
  clock_gettime(CLOCK_REALTIME, &t1);
  printf("Execution took %f seconds\n", diff(t0, t1));

  // Close all open files & clear
  pgOrders->finish();
  pgLineitem->finish();
  rawCatalog.clear();
}

void tpchJoin4(map<string, dataset> datasetCatalog, int predicate) {
  RawContext &ctx = *prepareContext("tpch-csv-join4");
  RawCatalog &rawCatalog = RawCatalog::getInstance();

  string nameLineitem = string("lineitem");
  dataset lineitem = datasetCatalog[nameLineitem];
  string nameOrders = string("orders");
  dataset orders = datasetCatalog[nameOrders];
  map<string, RecordAttribute *> argsLineitem = lineitem.recType.getArgsMap();
  map<string, RecordAttribute *> argsOrder = orders.recType.getArgsMap();

  /**
   * SCAN 1: Orders
   */
  string ordersNamePrefix = orders.path;
  RecordType recOrders = orders.recType;
  int policy = 5;
  int lineHint = orders.linehint;
  char delimInner = '|';
  vector<RecordAttribute *> orderProjections;
  RecordAttribute *o_orderkey = argsOrder["o_orderkey"];
  RecordAttribute *o_totalprice = argsOrder["o_totalprice"];
  orderProjections.push_back(o_orderkey);

  BinaryColPlugin *pgOrders =
      new BinaryColPlugin(&ctx, ordersNamePrefix, recOrders, orderProjections);
  rawCatalog.registerPlugin(ordersNamePrefix, pgOrders);
  Scan *scanOrders = new Scan(&ctx, *pgOrders);

  /**
   * SCAN 2: Lineitem
   */
  string lineitemNamePrefix = lineitem.path;
  RecordType recLineitem = lineitem.recType;
  policy = 5;
  lineHint = lineitem.linehint;
  delimInner = '|';
  vector<RecordAttribute *> lineitemProjections;
  RecordAttribute *l_orderkey = argsLineitem["l_orderkey"];
  RecordAttribute *l_extendedprice = argsLineitem["l_extendedprice"];
  lineitemProjections.push_back(l_orderkey);
  lineitemProjections.push_back(l_extendedprice);

  BinaryColPlugin *pgLineitem = new BinaryColPlugin(
      &ctx, lineitemNamePrefix, recLineitem, lineitemProjections);
  rawCatalog.registerPlugin(lineitemNamePrefix, pgLineitem);
  Scan *scanLineitem = new Scan(&ctx, *pgLineitem);

  /*
   * SELECT on LINEITEM
   */
  list<RecordAttribute> argProjectionsRight;
  argProjectionsRight.push_back(*l_orderkey);
  expressions::Expression *rightArg =
      new expressions::InputArgument(&recLineitem, 1, argProjectionsRight);
  expressions::Expression *lhs = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);
  expressions::Expression *rhs = new expressions::IntConstant(predicate);
  expressions::Expression *pred = new expressions::LtExpression(lhs, rhs);

  Select *sel = new Select(pred, scanLineitem);
  scanLineitem->setParent(sel);

  /**
   * JOIN
   */
  /* join key - orders */
  list<RecordAttribute> argProjectionsLeft;
  argProjectionsLeft.push_back(*o_orderkey);
  expressions::Expression *leftArg =
      new expressions::InputArgument(&recOrders, 0, argProjectionsLeft);

  expressions::Expression *leftPred = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);

  /* join key - lineitem */
  //    list<RecordAttribute> argProjectionsRight;
  //    argProjectionsRight.push_back(*l_orderkey);
  //    expressions::Expression* rightArg = new expressions::InputArgument(
  //            &recLineitem, 1, argProjectionsRight);
  expressions::Expression *rightPred = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);

  /* join pred. */
  expressions::BinaryExpression *joinPred =
      new expressions::EqExpression(leftPred, rightPred);

  /* left materializer - no explicit field needed - added them to help caching
   */
  vector<RecordAttribute *> fieldsLeft;
  fieldsLeft.push_back(o_orderkey);
  fieldsLeft.push_back(o_totalprice);
  vector<materialization_mode> outputModesLeft;
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);

  /* explicit mention to left OID */
  RecordAttribute *projTupleL =
      new RecordAttribute(ordersNamePrefix, activeLoop, pgOrders->getOIDType());
  vector<RecordAttribute *> OIDLeft;
  OIDLeft.push_back(projTupleL);
  expressions::Expression *exprLeftOID = new expressions::RecordProjection(
      pgOrders->getOIDType(), leftArg, *projTupleL);
  expressions::Expression *exprO_Orderkey = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);
  expressions::Expression *exprTotalprice = new expressions::RecordProjection(
      o_totalprice->getOriginalType(), leftArg, *o_totalprice);
  vector<expression_t> expressionsLeft;
  expressionsLeft.push_back(exprLeftOID);
  expressionsLeft.push_back(exprO_Orderkey);
  expressionsLeft.push_back(exprTotalprice);

  Materializer *matLeft =
      new Materializer(fieldsLeft, expressionsLeft, OIDLeft, outputModesLeft);

  /* right materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsRight;
  fieldsRight.push_back(l_orderkey);
  fieldsRight.push_back(l_extendedprice);
  vector<materialization_mode> outputModesRight;
  outputModesRight.insert(outputModesRight.begin(), EAGER);
  outputModesRight.insert(outputModesRight.begin(), EAGER);

  /* explicit mention to right OID */
  RecordAttribute *projTupleR = new RecordAttribute(
      lineitemNamePrefix, activeLoop, pgLineitem->getOIDType());
  vector<RecordAttribute *> OIDRight;
  OIDRight.push_back(projTupleR);
  expressions::Expression *exprRightOID = new expressions::RecordProjection(
      pgLineitem->getOIDType(), rightArg, *projTupleR);
  expressions::Expression *exprOrderkey = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);
  expressions::Expression *exprExtendedprice =
      new expressions::RecordProjection(l_extendedprice->getOriginalType(),
                                        rightArg, *l_extendedprice);
  vector<expression_t> expressionsRight;
  expressionsRight.push_back(exprRightOID);
  expressionsRight.push_back(exprOrderkey);
  expressionsRight.push_back(exprExtendedprice);

  Materializer *matRight = new Materializer(fieldsRight, expressionsRight,
                                            OIDRight, outputModesRight);

  char joinLabel[] = "radixJoin";
  RadixJoin *join = new RadixJoin(*joinPred, scanOrders, sel, &ctx, joinLabel,
                                  *matLeft, *matRight);

  scanOrders->setParent(join);
  sel->setParent(join);

  /**
   * REDUCE
   * MAX(orderkey)
   */
  /* Output: */

  vector<Monoid> accs;
  vector<expression_t> outputExprs;
  accs.push_back(MAX);
  accs.push_back(MAX);
  outputExprs.push_back(exprOrderkey);
  outputExprs.push_back(exprExtendedprice);

  opt::Reduce *reduce = new opt::Reduce(accs, outputExprs, true, join, &ctx);
  join->setParent(reduce);

  // Run function
  struct timespec t0, t1;
  clock_gettime(CLOCK_REALTIME, &t0);
  reduce->produce();
  ctx.prepareFunction(ctx.getGlobalFunction());
  clock_gettime(CLOCK_REALTIME, &t1);
  printf("Execution took %f seconds\n", diff(t0, t1));

  // Close all open files & clear
  pgOrders->finish();
  pgLineitem->finish();
  rawCatalog.clear();
}

void tpchJoinWarmupAll(map<string, dataset> datasetCatalog) {
  RawContext &ctx = *prepareContext("tpch-csv-join1a");
  RawCatalog &rawCatalog = RawCatalog::getInstance();

  string nameLineitem = string("lineitem");
  dataset lineitem = datasetCatalog[nameLineitem];
  string nameOrders = string("orders");
  dataset orders = datasetCatalog[nameOrders];
  map<string, RecordAttribute *> argsLineitem = lineitem.recType.getArgsMap();
  map<string, RecordAttribute *> argsOrder = orders.recType.getArgsMap();

  /**
   * SCAN 1: Orders
   */
  string ordersNamePrefix = orders.path;
  RecordType recOrders = orders.recType;
  int policy = 5;
  int lineHint = orders.linehint;
  char delimInner = '|';
  vector<RecordAttribute *> orderProjections;
  RecordAttribute *o_orderkey = argsOrder["o_orderkey"];
  RecordAttribute *o_totalprice = argsOrder["o_totalprice"];
  orderProjections.push_back(o_orderkey);
  orderProjections.push_back(o_totalprice);

  BinaryColPlugin *pgOrders =
      new BinaryColPlugin(&ctx, ordersNamePrefix, recOrders, orderProjections);
  rawCatalog.registerPlugin(ordersNamePrefix, pgOrders);
  Scan *scanOrders = new Scan(&ctx, *pgOrders);

  /**
   * SCAN 2: Lineitem
   */
  string lineitemNamePrefix = lineitem.path;
  RecordType recLineitem = lineitem.recType;
  policy = 5;
  lineHint = lineitem.linehint;
  delimInner = '|';
  vector<RecordAttribute *> lineitemProjections;
  RecordAttribute *l_orderkey = argsLineitem["l_orderkey"];
  RecordAttribute *l_extendedprice = argsLineitem["l_extendedprice"];
  lineitemProjections.push_back(l_orderkey);
  lineitemProjections.push_back(l_extendedprice);

  BinaryColPlugin *pgLineitem = new BinaryColPlugin(
      &ctx, lineitemNamePrefix, recLineitem, lineitemProjections);
  rawCatalog.registerPlugin(lineitemNamePrefix, pgLineitem);
  Scan *scanLineitem = new Scan(&ctx, *pgLineitem);

  list<RecordAttribute> argProjectionsRight;
  argProjectionsRight.push_back(*l_orderkey);
  expressions::Expression *rightArg =
      new expressions::InputArgument(&recLineitem, 1, argProjectionsRight);

  /**
   * JOIN
   */
  /* join key - orders */
  list<RecordAttribute> argProjectionsLeft;
  argProjectionsLeft.push_back(*o_orderkey);
  expressions::Expression *leftArg =
      new expressions::InputArgument(&recOrders, 0, argProjectionsLeft);
  expressions::Expression *leftPred = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);

  /* join key - lineitem */
  expressions::Expression *rightPred = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);

  /* join pred. */
  expressions::BinaryExpression *joinPred =
      new expressions::EqExpression(leftPred, rightPred);

  /* left materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsLeft;
  fieldsLeft.push_back(o_orderkey);
  fieldsLeft.push_back(o_totalprice);
  vector<materialization_mode> outputModesLeft;
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);
  outputModesLeft.insert(outputModesLeft.begin(), EAGER);

  /* explicit mention to left OID */
  RecordAttribute *projTupleL =
      new RecordAttribute(ordersNamePrefix, activeLoop, pgOrders->getOIDType());
  vector<RecordAttribute *> OIDLeft;
  OIDLeft.push_back(projTupleL);
  expressions::Expression *exprLeftOID = new expressions::RecordProjection(
      pgOrders->getOIDType(), leftArg, *projTupleL);
  expressions::Expression *exprO_Orderkey = new expressions::RecordProjection(
      o_orderkey->getOriginalType(), leftArg, *o_orderkey);
  expressions::Expression *exprTotalprice = new expressions::RecordProjection(
      o_totalprice->getOriginalType(), leftArg, *o_totalprice);
  vector<expression_t> expressionsLeft;
  expressionsLeft.push_back(exprLeftOID);
  expressionsLeft.push_back(exprO_Orderkey);
  expressionsLeft.push_back(exprTotalprice);

  Materializer *matLeft =
      new Materializer(fieldsLeft, expressionsLeft, OIDLeft, outputModesLeft);

  /* right materializer - no explicit field needed */
  vector<RecordAttribute *> fieldsRight;
  fieldsRight.push_back(l_orderkey);
  fieldsRight.push_back(l_extendedprice);
  vector<materialization_mode> outputModesRight;
  outputModesRight.insert(outputModesRight.begin(), EAGER);
  outputModesRight.insert(outputModesRight.begin(), EAGER);

  /* explicit mention to right OID */
  RecordAttribute *projTupleR = new RecordAttribute(
      lineitemNamePrefix, activeLoop, pgLineitem->getOIDType());
  vector<RecordAttribute *> OIDRight;
  OIDRight.push_back(projTupleR);
  expressions::Expression *exprRightOID = new expressions::RecordProjection(
      pgLineitem->getOIDType(), rightArg, *projTupleR);
  expressions::Expression *exprL_Orderkey = new expressions::RecordProjection(
      l_orderkey->getOriginalType(), rightArg, *l_orderkey);
  expressions::Expression *exprExtendedprice =
      new expressions::RecordProjection(l_extendedprice->getOriginalType(),
                                        rightArg, *l_extendedprice);
  vector<expression_t> expressionsRight;
  expressionsRight.push_back(exprRightOID);
  expressionsRight.push_back(exprL_Orderkey);
  expressionsRight.push_back(exprExtendedprice);

  Materializer *matRight = new Materializer(fieldsRight, expressionsRight,
                                            OIDRight, outputModesRight);

  char joinLabel[] = "radixJoin";
  RadixJoin *join = new RadixJoin(*joinPred, scanOrders, scanLineitem, &ctx,
                                  joinLabel, *matLeft, *matRight);
  scanOrders->setParent(join);
  scanLineitem->setParent(join);

  /**
   * REDUCE
   * COUNT(*)
   */
  /* Output: */
  expressions::Expression *outputExpr = new expressions::IntConstant(1);
  ReduceNoPred *reduce = new ReduceNoPred(SUM, outputExpr, join, &ctx);
  join->setParent(reduce);

  // Run function
  struct timespec t0, t1;
  clock_gettime(CLOCK_REALTIME, &t0);
  reduce->produce();
  ctx.prepareFunction(ctx.getGlobalFunction());
  clock_gettime(CLOCK_REALTIME, &t1);
  printf("Execution took %f seconds\n", diff(t0, t1));

  // Close all open files & clear
  pgOrders->finish();
  pgLineitem->finish();
  rawCatalog.clear();
}
