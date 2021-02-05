/*
 * TCMachineTeamInfo.cpp
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2018 Apple Inc. and the FoundationDB project authors
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

#include "TCMachineTeamInfo.h"

#include <sstream>

TCMachineTeamInfo::TCMachineTeamInfo(vector<Reference<TCMachineInfo>> const& machines)
  : machines(machines), id(deterministicRandom()->randomUniqueID()) {
	machineIDs.reserve(machines.size());
	for (int i = 0; i < machines.size(); i++) {
		machineIDs.push_back(machines[i]->machineID);
	}
	sort(machineIDs.begin(), machineIDs.end());
}

int TCMachineTeamInfo::size() const {
	ASSERT(machines.size() == machineIDs.size());
	return machineIDs.size();
}

std::string TCMachineTeamInfo::getMachineIDsStr() const {
	std::stringstream ss;

	if (machineIDs.empty()) return "[unset]";

	for (const auto& id : machineIDs) {
		ss << id.contents().toString() << " ";
	}

	return std::move(ss).str();
}

bool TCMachineTeamInfo::operator==(const TCMachineTeamInfo& rhs) const {
	return this->machineIDs == rhs.machineIDs;
}
