package ch.epfl.dias.calcite.adapter.pelago.rules;

import org.apache.calcite.plan.RelOptRule;
import org.apache.calcite.plan.RelOptRuleCall;
import org.apache.calcite.plan.RelTraitSet;
import org.apache.calcite.rel.RelDistribution;
import org.apache.calcite.rel.RelDistributions;
import org.apache.calcite.rel.RelNode;

import com.google.common.collect.ImmutableMap;

import ch.epfl.dias.calcite.adapter.pelago.PelagoDeviceCross;
import ch.epfl.dias.calcite.adapter.pelago.PelagoJoin;
import ch.epfl.dias.calcite.adapter.pelago.PelagoRouter;
import ch.epfl.dias.calcite.adapter.pelago.PelagoSplit;
import ch.epfl.dias.calcite.adapter.pelago.RelDeviceType;
import ch.epfl.dias.calcite.adapter.pelago.RelDeviceTypeTraitDef;
import ch.epfl.dias.calcite.adapter.pelago.RelHomDistribution;

public class PelagoPushRouterBelowJoin extends RelOptRule {
  public static final PelagoPushRouterBelowJoin INSTANCE = new PelagoPushRouterBelowJoin();

  protected PelagoPushRouterBelowJoin() {
    super(
      operand(
        PelagoRouter.class,
        operand(
          PelagoJoin.class,
          any()
        )
      )
    );
  }

  public boolean matches(RelOptRuleCall call) {
    return ((PelagoRouter) call.rel(0)).getHomDistribution() == RelHomDistribution.RANDOM;
  }

  public void onMatch(RelOptRuleCall call) {
    PelagoJoin   join   = call.rel(1);
    RelNode      build  = join.getLeft();
    RelNode      probe  = join.getRight();

    RelNode new_build = convert(
      build,
      build.getTraitSet().replace(RelHomDistribution.BRDCST).replace(join.getTraitSet().getTrait(RelDeviceTypeTraitDef.INSTANCE))
    );

    RelNode new_probe = convert(
      probe,
      probe.getTraitSet().replace(RelHomDistribution.RANDOM).replace(join.getTraitSet().getTrait(RelDeviceTypeTraitDef.INSTANCE))
    );

    call.transformTo(
      join.copy(
        null,
        join.getCondition(),
        convert(new_build, join.getTraitSet().getTrait(RelDeviceTypeTraitDef.INSTANCE)),
        convert(new_probe, join.getTraitSet().getTrait(RelDeviceTypeTraitDef.INSTANCE)),
        join.getJoinType(),
        join.isSemiJoinDone()
      )
    );
  }
}
