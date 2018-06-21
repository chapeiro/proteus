package ch.epfl.dias.calcite.adapter.pelago.rules;

import ch.epfl.dias.calcite.adapter.pelago.PelagoRel;
import ch.epfl.dias.calcite.adapter.pelago.PelagoToEnumerableConverter;
//import ch.epfl.dias.calcite.adapter.pelago.trait.RelDeviceType;
import org.apache.calcite.adapter.enumerable.EnumerableConvention;
import org.apache.calcite.plan.RelOptRuleCall;
import org.apache.calcite.plan.RelTraitSet;
import org.apache.calcite.rel.RelDeviceType;
import org.apache.calcite.rel.RelDeviceTypeTraitDef;
import org.apache.calcite.rel.RelDistribution;
import org.apache.calcite.rel.RelDistributionTraitDef;
import org.apache.calcite.rel.RelDistributions;
import org.apache.calcite.rel.RelNode;
import org.apache.calcite.rel.convert.ConverterRule;
import org.apache.calcite.rel.core.RelFactories;
import org.apache.calcite.tools.RelBuilderFactory;

import com.google.common.base.Predicates;
import com.google.common.base.Supplier;

/**
 * Rule to convert a relational expression from
 * {@link PelagoRel#CONVENTION} to {@link EnumerableConvention}.
 */
public class PelagoToEnumerableConverterRule extends ConverterRule {
    public static final ConverterRule INSTANCE =
            new PelagoToEnumerableConverterRule(RelFactories.LOGICAL_BUILDER);

    /**
     * Creates a CassandraToEnumerableConverterRule.
     *
     * @param relBuilderFactory Builder for relational expressions
     */
    public PelagoToEnumerableConverterRule(
            RelBuilderFactory relBuilderFactory) {
        super(RelNode.class, PelagoRel.CONVENTION, EnumerableConvention.INSTANCE,
                "PelagoToEnumerableConverterRule");
    }

    @Override public RelNode convert(RelNode rel) {
//        RelTraitSet newTraitSet = rel.getTraitSet().replace(getOutConvention()); //.replace(RelDeviceType.ANY);
//        RelNode inp = rel;
//        RelNode inp = RelDistributionTraitDef.INSTANCE.convert(rel.getCluster().getPlanner(), rel, RelDistributions.SINGLETON, true);
//        RelNode inp = LogicalExchange.create(rel, RelDistributions.SINGLETON);
//        System.out.println(inp.getTraitSet());

//        RelTraitSet traitSet = rel.getCluster().traitSet().replace(PelagoRel.CONVENTION)
//                .replaceIf(RelDistributionTraitDef.INSTANCE, new Supplier<RelDistribution>() {
//                    public RelDistribution get() {
//                        return RelDistributions.SINGLETON;
//                    }
//                }).replaceIf(RelDeviceTypeTraitDef.INSTANCE, new Supplier<RelDeviceType>() {
//                    public RelDeviceType get() {
//                        return RelDeviceType.X86_64;
//                    }
//                });

        RelNode inp = rel;//convert(convert(rel, RelDistributions.SINGLETON), RelDeviceType.X86_64); //Convert to sequential
//        RelNode inp = convert(rel, traitSet);

        RelNode tmp = PelagoToEnumerableConverter.create(inp);
        return tmp;
    }

    public boolean matches(RelOptRuleCall call) {
//        return true;
//        if (!call.rel(0).getTraitSet().satisfies(RelTraitSet.createEmpty().plus(RelDistributions.SINGLETON))) return false;
        if (!call.rel(0).getTraitSet().contains(RelDeviceType.X86_64)) return false;
        return true;
    }
}