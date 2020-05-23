/*
 * LowLatencyCommit.actor.cpp
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2020 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fdbrpc/ContinuousSample.h"
#include "fdbclient/NativeAPI.actor.h"
#include "fdbserver/TesterInterface.actor.h"
#include "fdbclient/ReadYourWrites.h"
#include "fdbserver/Knobs.h"
#include "fdbserver/workloads/workloads.actor.h"
#include "flow/Arena.h"
#include "flow/actorcompiler.h" // This must be the last #include.

struct LowLatencyCommitWorkload : TestWorkload {

	static constexpr uint64_t debugPrefix = 0xabcdabcdll << 32;

	double testDuration;
	double delayBetweenTransactions;
	int keySize;
	int valueSize;
	int writesPerTransaction;
	bool getReadVersionBeforeCommit;
	ContinuousSample<double> latencies;
	double debugTransactionRate = 1.0;
	int totalTransactions = 0;

	LowLatencyCommitWorkload(WorkloadContext const& wcx) : TestWorkload(wcx), latencies(10000) {
		testDuration = getOption(options, LiteralStringRef("testDuration"), 60.0);
		delayBetweenTransactions = getOption(options, LiteralStringRef("delayBetweenTransactions"), 0.0);
		keySize = getOption(options, LiteralStringRef("keySize"), 16);
		valueSize = getOption(options, LiteralStringRef("valueSize"), 16);
		writesPerTransaction = getOption(options, LiteralStringRef("writesPerTransaction"), 1);
		getReadVersionBeforeCommit = getOption(options, LiteralStringRef("getReadVersionBeforeCommit"), true);
		debugTransactionRate = getOption(options, LiteralStringRef("debugTransactionRate"), 1.0);
	}

	virtual std::string description() override { return "LowLatencyCommit"; }

	virtual Future<Void> setup(Database const& cx) override { return Void(); }

	virtual Future<Void> start(Database const& cx) override { return clientId ? Void() : _start(cx, this); }

	ACTOR static Future<Void> _start(Database cx, LowLatencyCommitWorkload* self) {
		state double testStart = now();
		try {
			loop {
				wait(delay(self->delayBetweenTransactions));
				state Transaction tr(cx);
				loop {
					if (deterministicRandom()->random01() < self->debugTransactionRate) {
						UID debugID(debugPrefix | deterministicRandom()->randomInt64(0, 1ll << 32), 0);
						tr.debugTransaction(debugID);
					}
					try {
						if (self->getReadVersionBeforeCommit) wait(success(tr.getReadVersion()));
						for (int i = 0; i < self->writesPerTransaction; ++i) {
							Key key =
							    LiteralStringRef(deterministicRandom()->randomAlphaNumeric(self->keySize).c_str());
							Value value =
							    LiteralStringRef(deterministicRandom()->randomAlphaNumeric(self->keySize).c_str());
							tr.set(key, value);
						}
						state double start = g_network->now();
						wait(tr.commit());
						self->latencies.addSample(g_network->now() - start);
						++self->totalTransactions;
						break;
					} catch (Error& e) {
						wait(tr.onError(e));
					}
				}
				if (now() - testStart > self->testDuration) break;
			}
			return Void();
		} catch (Error& e) {
			TraceEvent(SevError, "LowLatencyCommitError").error(e, true);
			throw;
		}
	}

	virtual Future<bool> check(Database const& cx) override { return true; }

	virtual void getMetrics(vector<PerfMetric>& m) override {
		if (clientId > 0) return;
		m.push_back(PerfMetric("Mean latency", latencies.mean(), false));
		m.push_back(PerfMetric("Min latency", latencies.min(), false));
		m.push_back(PerfMetric("1% latency", latencies.percentile(0.01), false));
		m.push_back(PerfMetric("50% latency", latencies.percentile(0.5), false));
		m.push_back(PerfMetric("90% latency", latencies.percentile(0.9), false));
		m.push_back(PerfMetric("99% latency", latencies.percentile(0.99), false));
		m.push_back(PerfMetric("99.9% latency", latencies.percentile(0.999), false));
		m.push_back(PerfMetric("Max latency", latencies.max(), false));
		m.push_back(PerfMetric("Total transactions", totalTransactions, false));
	}
};

WorkloadFactory<LowLatencyCommitWorkload> LowLatencyCommitWorkloadFactory("LowLatencyCommit");
