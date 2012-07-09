// See MultiWithConversion.h for documentation
// This header is included at the end of RSIM_Templates.h

#include "YicesSolver.h"

MULTI_DOMAIN_TEMPLATE
void
MultiDomainDemoPolicy<State, ValueType>::init()
{
    // We can't call anything here that uses the policy's RSIM_Thread because the thread is not fully initialized yet.
    name = "MultiDomainDemoPolicy";
    std::cerr <<"RSIM is using MultiDomainDemoPolicy (see demos/MultiWithConversion.h)\n";

    // By default, disable all our sub-domains.  Only allow the simulator's domain to be active.
    this->set_active_policies(CONCRETE.mask);

#if 1
    // Do we want to use an SMT solver for the symbolic semantics policy?  Doing so will make it much slower, but will also
    // allow us to answer more complex questions.  Regardless of whether ROSE's SymbolicSemantics uses an SMT solver, you will
    // always be able to use the SMTSolver class (or YicesSolver subclass) to build your own SMT queries. See ROSE doxygen
    // documentation for the SMTSolver class and example code in SymbolicSemantics.h.
    YicesSolver *yices = new YicesSolver;
    //yices->set_linkage(YicesSolver::LM_LIBRARY); // much faster, but has fewer debugging capabilities
    yices->set_debug(stderr); // will show you solver input and output for LM_EXECUTABLE linkage
    this->get_policy(SYMBOLIC).set_solver(yices);
#endif

#if 0
    // Do you want ROSE to prune McCarthy memory expressions to eliminate clauses whose condition can't be satisfied? Pruning
    // is the default.  Turning it off will cause fewer calls to the SMT solver but the remaining calls could have much larger
    // expressions.
    this->get_policy(SYMBOLIC).disable_read_pruning();
#endif
}

MULTI_DOMAIN_TEMPLATE
RTS_Message *
MultiDomainDemoPolicy<State, ValueType>::trace()
{
    return this->get_policy(CONCRETE).thread->tracing(TRACE_MISC);
}

MULTI_DOMAIN_TEMPLATE
void
MultiDomainDemoPolicy<State, ValueType>::trigger(rose_addr_t target_va)
{
    trace()->mesg("%s: triggered; enabling all sub-domains; branching to 0x%"PRIx64, name, target_va);
    this->set_active_policies(allowed_policies);
    this->writeRegister("eip", RSIM_SEMANTICS_VTYPE<32>(target_va));
    triggered = true;
}

MULTI_DOMAIN_TEMPLATE
void
MultiDomainDemoPolicy<State, ValueType>::startInstruction(SgAsmInstruction *insn_)
{
    if (triggered) {
        SgAsmx86Instruction *insn = isSgAsmx86Instruction(insn_);
        assert(insn!=NULL);

        // Print the instruction being executed.  You can also use "--debug=insn", but that prints *every* instruction, not
        // just the ones after we jump to an arbitrary offset.
        trace()->mesg("%s: executing: %s", name, unparseInstruction(insn).c_str());

        // This shows how you can disable a domain based on some condition.  This example looks at all the register expressions
        // and counts how many nodes are in the expression tree.  If the some is larger than some arbitrary amount, we
        // permanently turn off the symbolic semantics.  You can turn it on later, but you'll probably want to reinitialize its
        // state (current and initial) if you do so.
        if (0!=(allowed_policies & SYMBOLIC.mask)) {
            size_t sym_cplx = symbolic_state_complexity();
            if (sym_cplx > 10000) {
                trace()->mesg("%s: symbolic complexity = %zu; disabling symbolic semantics", name, sym_cplx);
                allowed_policies &= ~SYMBOLIC.mask;
            }
        }

        // Here's how you can decide which sub-domains should be active based on the kind of instruction.
        switch (insn->get_kind()) {
            case x86_int:
                // If we want interrupts to only execute in the simulator concrete domain, we can disable all the other
                // sub-domains.  This isn't actually necessary for INT because the interval domain and symbolic domain already do
                // nothing for the INT instruction.  The only difference is that disabling those domains (like we do here) even
                // disables the update of the EIP register in those domains.  See ROSE's MultiSemantics<>::Policy for documentation
                // of set_active_policies().  The sub-domains are numbered as described at the top of MultiWithConversion.h.
                trace()->mesg("%s: got an INT instruction", name);
                this->set_active_policies(CONCRETE.mask);
                break;
            default:
                // Everything else should execute in all domains.
                this->set_active_policies(allowed_policies);
                break;
        }

        // The simulator's concrete domain is driving the instruction fetch-excute loop via its EIP register (although
        // our analysis might be monkeying with the EIP value).  Most of the instruction semantics policies check that
        // the EIP register is consistent with the instruction's address each time an instruction is executed, so it's
        // good practice to make sure that the check will always succeed. We do that by explicitly writing the
        // instruction's address into the EIP register across all enabled semantic domains.
        this->writeRegister("eip", RSIM_SEMANTICS_VTYPE<32>(insn->get_address()));

        // This paragraph shows how you can get some statistics about how the SMT solver is being used.  The input and output
        // sizes will always be zero if you're using LM_LIBRARY linkage (see MultiDomainDemoPolicy::init).  You can also get
        // class-wide statistics by using get_class_stats() and reset_class_stats().  See ROSE doxygen documentation for the
        // SMTSolver class.
        static size_t ncalls = 0;
        static const size_t report_interval = 100;
        if (0==++ncalls % report_interval) {
            SMTSolver *solver = this->get_policy(SYMBOLIC).get_solver();
            if (solver) {
                const SMTSolver::Stats &stats = solver->get_stats();
                if (stats.ncalls>0) {
                    trace()->mesg("%s: SMT solver stats since last report: ncalls=%zu, input=%zu bytes, output=%zu bytes",
                                  name, stats.ncalls, stats.input_size, stats.output_size);
                    solver->reset_stats();
                }
            }
        }
    }
    Super::startInstruction(insn_);
}

MULTI_DOMAIN_TEMPLATE
void
MultiDomainDemoPolicy<State, ValueType>::finishInstruction(SgAsmInstruction *insn)
{
    Super::finishInstruction(insn);
    if (triggered) {
        // Here's how you could get a trace showing the complete machine state in every semantic domain. The simulator's
        // "--debug=insn,state" does something similar but only shows the concrete domain.  If you want a heading above each
        // domain's state then you should print them individually.
        std::ostringstream ss; ss <<*this;
        trace()->mesg("%s: complete state after %s\n%s",
                      name, unparseInstruction(insn).c_str(), StringUtility::prefixLines(ss.str(), "    ").c_str());

#if 0 /* This is very noisy! */
        // Here's how you can print an individual state.  BTW, printing a policy by default prints the policies state. Most
        // policies implement a get_state() method if you really need the state itself.
        std::ostringstream s2; s2 <<this->get_policy(INTERVAL);
        trace()->mesg("%s: interval state after %s\n%s",
                      name, unparseInstruction(insn).c_str(), StringUtility::prefixLines(s2.str(), "    ").c_str());
#endif
    }
}

MULTI_DOMAIN_TEMPLATE
template<size_t nBits>
ValueType<nBits>
MultiDomainDemoPolicy<State, ValueType>::xor_(const ValueType<nBits> &a, const ValueType<nBits> &b)
{
    ValueType<nBits> retval = Super::xor_(a, b);
    if (triggered) {
        // A common optimization is to zero a register by xoring it with itself. However, the IntervalSemantics' xor_() method
        // can't detect this case because all it sees is two operands which, although their ranges are identical, could have
        // different values.  So we use the symbolic semantics: if the symbolic semantics says the answer is zero then we set
        // the interval semantics' value to the singleton set {0}.  Simply looking at the nBit-symbolic answer won't easily
        // tell us if it must be zero (it could be a complex expression), but by calling equalToZero() we cause the SMT solver
        // to be invoked.
        if (this->is_active(SYMBOLIC) && retval.is_valid(SYMBOLIC)) {
            SYMBOLIC_VALUE<1> symbolic_bool = this->get_policy(SYMBOLIC).equalToZero(retval.get_subvalue(SYMBOLIC));
            if (symbolic_bool.is_known() && symbolic_bool.known_value()!=0) {
                trace()->mesg("%s: xor_ optimization triggered for interval domain", name);
                trace()->mesg("%s: instruction: %s", name, unparseInstruction(this->get_policy(CONCRETE).get_insn()).c_str());
#if 0 // FIXME: throws an exception, segfaults, ?
                std::ostringstream ss; ss <<this->get_policy(SYMBOLIC);
                trace()->mesg("%s: symbolic state:\n%s", name, StringUtility::prefixLines(ss.str(), "    ").c_str());
#endif
                retval.set_subvalue(INTERVAL, INTERVAL_VALUE<nBits>(0));
            }
        }
    }
    return retval;
}

MULTI_DOMAIN_TEMPLATE
size_t
MultiDomainDemoPolicy<State, ValueType>::symbolic_state_complexity()
{
    const RegisterDictionary::Entries &regs = this->get_policy(SYMBOLIC).get_register_dictionary()->get_registers();
    ExprNodeCounter visitor;
    for (RegisterDictionary::Entries::const_iterator ri=regs.begin(); ri!=regs.end(); ++ri) {
        try {
            const RegisterDescriptor &reg = ri->second;
            switch (reg.get_nbits()) { // arghh!  side effect of using templates :-/
                case 1:  symbolic_expr_complexity(this->get_policy(SYMBOLIC).template readRegister<1>(reg),  &visitor); break;
                case 8:  symbolic_expr_complexity(this->get_policy(SYMBOLIC).template readRegister<8>(reg),  &visitor); break;
                case 16: symbolic_expr_complexity(this->get_policy(SYMBOLIC).template readRegister<16>(reg), &visitor); break;
                case 32: symbolic_expr_complexity(this->get_policy(SYMBOLIC).template readRegister<32>(reg), &visitor); break;
                default: break; // skip these registers
            }
        } catch (SymbolicPolicy::Exception &e) {
            // register is probably not implemented in the state object, so skip it
        }
    }
    return visitor.nnodes;
}

MULTI_DOMAIN_TEMPLATE
template<size_t nBits>
ValueType<nBits>
MultiDomainDemoPolicy<State, ValueType>::readMemory(X86SegmentRegister sr, ValueType<32> addr, const ValueType<1> &cond)
{
    if (!triggered)
        return  Super::template readMemory<nBits>(sr, addr, cond);

    // We need a symbolic address. If we don't have one, then try to construct one from values we do have. (Andreas, this
    // is the stuff you guys need to write--how to convert a value from one domain to another; I just stubbed this out for
    // now. [RPM])
    if (!addr.is_valid(SYMBOLIC)) {
        if (addr.is_valid(INTERVAL)) {
            // FIXME: scan through the possible address intervals and build an expression.  The interval semantics
            // interface and the symbolic interface should already have the methods necessary.
        } else if (addr.is_valid(CONCRETE)) {
            assert(addr.get_subvalue(CONCRETE).is_known());
            addr.set_subvalue(SYMBOLIC, SYMBOLIC_VALUE<32>(addr.get_subvalue(CONCRETE).known_value()));
        }
        assert(addr.is_valid(SYMBOLIC));
    }

    // Read a multi-byte value from memory in little-endian order.
    assert(8==nBits || 16==nBits || 32==nBits);
    SYMBOLIC_VALUE<32> a0 = addr.get_subvalue(SYMBOLIC);
    ValueType<32> dword = this->concat(ValueType<24>(0), state.mem_read_byte(a0));
    if (nBits>=16) {
        SYMBOLIC_VALUE<32> a1 = this->get_policy(SYMBOLIC).add(a0, SYMBOLIC_VALUE<32>(1));
        dword = this->or_(dword, this->concat(ValueType<16>(0),
                                              this->concat(state.mem_read_byte(a1), ValueType<8>(0))));
    }
    if (nBits>=24) {
        SYMBOLIC_VALUE<32> a2 = this->get_policy(SYMBOLIC).add(a0, SYMBOLIC_VALUE<32>(2));
        dword = this->or_(dword, this->concat(ValueType<8>(0),
                                              this->concat(state.mem_read_byte(a2), ValueType<16>(0))));
    }
    if (nBits>=32) {
        SYMBOLIC_VALUE<32> a3 = this->get_policy(SYMBOLIC).add(a0, SYMBOLIC_VALUE<32>(3));
        dword = this->or_(dword, this->concat(state.mem_read_byte(a3), ValueType<24>(0)));
    }
    return this->template extract<0, nBits>(dword);
}

MULTI_DOMAIN_TEMPLATE
template<size_t nBits>
void
MultiDomainDemoPolicy<State, ValueType>::writeMemory(X86SegmentRegister sr, ValueType<32> addr,
                                                     const ValueType<nBits> &data, const ValueType<1> &cond)
{
    // The concrete state should always do its own thing.
    unsigned old_policies = this->get_active_policies();
    try {
        this->set_active_policies(CONCRETE.mask);
        Super::template writeMemory<nBits>(sr, addr, data, cond);
        this->set_active_policies(old_policies);
    } catch (...) {
        this->set_active_policies(old_policies);
        throw;
    }

    // Do something special for our own domains
    if (triggered) {
        // We need a symbolic address. Cut-n-pasted from readMemory. FIXME
        if (!addr.is_valid(SYMBOLIC)) {
            if (addr.is_valid(INTERVAL)) {
                // FIXME: scan through the possible address intervals and build an expression.  The interval semantics
                // interface and the symbolic interface should already have the methods necessary.
            } else if (addr.is_valid(CONCRETE)) {
                assert(addr.get_subvalue(CONCRETE).is_known());
                addr.set_subvalue(SYMBOLIC, SYMBOLIC_VALUE<32>(addr.get_subvalue(CONCRETE).known_value()));
            }
        }

        // Add the address/value pair to the mixed-semantics memory state, one byte at a time in little-endian order.
        if (addr.is_valid(SYMBOLIC)) {
            assert(8==nBits || 16==nBits || 32==nBits);
            SYMBOLIC_VALUE<32> a0 = addr.get_subvalue(SYMBOLIC);
            ValueType<8> b0 = this->template extract<0, 8>(data);
            state.mem_write_byte(a0, b0);
            if (nBits>=16) {
                SYMBOLIC_VALUE<32> a1 = this->get_policy(SYMBOLIC).add(a0, SYMBOLIC_VALUE<32>(1));
                ValueType<8> b1 = this->template extract<8, 16>(data);
                state.mem_write_byte(a1, b1);
            }
            if (nBits>=24) {
                SYMBOLIC_VALUE<32> a2 = this->get_policy(SYMBOLIC).add(a0, SYMBOLIC_VALUE<32>(2));
                ValueType<8> b2 = this->template extract<16, 24>(data);
                state.mem_write_byte(a2, b2);
            }
            if (nBits>=32) {
                SYMBOLIC_VALUE<32> a3 = this->get_policy(SYMBOLIC).add(a0, SYMBOLIC_VALUE<32>(3));
                ValueType<8> b3 = this->template extract<24, 32>(data);
                state.mem_write_byte(a3, b3);
            }
        }
    }
}

template <template <size_t> class ValueType>
void
MultiDomainDemoState<ValueType>::mem_write_byte(const SYMBOLIC_VALUE<32> &addr, const ValueType<8> &value)
{
    memory.push_front(MemoryCell(addr, value));
}

template <template <size_t> class ValueType>
ValueType<8>
MultiDomainDemoState<ValueType>::mem_read_byte(const SYMBOLIC_VALUE<32> &addr)
{
    return ValueType<8>(); // FIXME: should return a McCarthy expression based on the cell list
}
