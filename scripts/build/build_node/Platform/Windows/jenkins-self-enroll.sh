#!/bin/bash
 
 
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 
set +x
 
get_metadata () {
    local response=$(curl -GLsX GET http://169.254.169.254/latest/meta-data/$1)
    echo "$response"
}
 
# Python is being used to parse the JSON response as jq on cygwin is mostly abandoned
get_secrets () {
    local sec_response=$(aws secretsmanager get-secret-value --secret-id $1 --query 'SecretString' --output text | python -c 'import sys, json;print(json.load(sys.stdin)["$2"])')
    echo "$sec_response"
}
 
# Get tag variables. This exports all instance tags as environment variables
INSTANCE_ID=$(get_metadata "instance-id")
REGION=$(get_metadata "placement/availability-zone" | sed 's/.$//')
export_statement=$(aws ec2 describe-tags --region "${REGION}" \
                        --filters "Name=resource-id,Values=${INSTANCE_ID}" \
                        --query 'Tags[?!contains(Key, `:`)].[Key,Value]' \
                        --output text | \
                        sed -E 's/^([^\s\t]+)[\s\t]+([^\n]+)$/export \1="\2"/g')
eval "${export_statement}"
 
INSTANCE_NAME="${Name}"
 
# Get instance data
INSTANCE_PRIVATE_IP=$(get_metadata "local-ipv4")
INSTANCE_PUBLIC_IP=$(get_metadata "public-ipv4")
CONTROLLER_USERNAME=$(get_secrets "${JENKINS_SECRET_ID}" "Username")
CONTROLLER_PASSWORD=$(get_secrets "${JENKINS_SECRET_ID}" "JenkinsAPIToken")
NODE_NAME="EC2 (${REGION}) - ${INSTANCE_NAME} (${INSTANCE_ID})"
NODE_WORKSPACE="c:/ly"
 
# Grab the private IP of the load balancer to allow the EC2 instance to use the security group allowlist (if the tag exists on a public IP instance)
if [ ! -z ${JENKINS_ELB} ]; then
    jenkinsip=$(aws ec2 describe-network-interfaces --filters Name=description,Values="ELB ${JENKINS_ELB}" --query 'NetworkInterfaces[*].PrivateIpAddresses[*].PrivateIpAddress' --output text | head -n1 | tr -d '\r')
     
    # Create a new hosts entry for the private IP
    echo "${jenkinsip} ${JENKINS_URL}" | tee -a /cygdrive/c/Windows/System32/drivers/etc/hosts
fi
 
# Download CLI jar from the controller
curl "${JENKINS_URL}"/jnlpJars/jenkins-cli.jar -o "${NODE_WORKSPACE}"/jenkins-cli.jar
curl "${JENKINS_URL}"/jnlpJars/agent.jar -o "${NODE_WORKSPACE}"/agent.jar
 
# Create node if it doesn't exists. Fail with true if it does exist
cat <<EOF | java -jar ${NODE_WORKSPACE}/jenkins-cli.jar -auth "${CONTROLLER_USERNAME}:${CONTROLLER_PASSWORD}" -s "${JENKINS_URL}" create-node "${NODE_NAME}" |java -jar ${NODE_WORKSPACE}/jenkins-cli.jar -auth "${CONTROLLER_USERNAME}:${CONTROLLER_PASSWORD}" -s "${JENKINS_URL}" update-node "${NODE_NAME}"
<slave>
  <name>${NODE_NAME}</name>
  <description>Ext IP: ${INSTANCE_PUBLIC_IP} Int IP: ${INSTANCE_PRIVATE_IP}</description>
  <remoteFS>${NODE_WORKSPACE}</remoteFS>
  <numExecutors>1</numExecutors>
  <mode>EXCLUSIVE</mode>
  <retentionStrategy class="hudson.slaves.RetentionStrategy$Always"/>
  <launcher class="hudson.slaves.JNLPLauncher">
    <tunnel>${JENKINS_JNLP_IP}</tunnel>
    <workDirSettings>
      <disabled>true</disabled>
      <internalDir>remoting</internalDir>
      <failIfWorkDirIsMissing>false</failIfWorkDirIsMissing>
    </workDirSettings>
  </launcher>
  <label>${NODE_LABEL}</label>
  <nodeProperties>
    <hudson.slaves.EnvironmentVariablesNodeProperty>
      <envVars serialization="custom">
        <unserializable-parents/>
        <tree-map>
          <default>
            <comparator class="hudson.util.CaseInsensitiveComparator"/>
          </default>
          <int>0</int>
        </tree-map>
      </envVars>
    </hudson.slaves.EnvironmentVariablesNodeProperty>
  </nodeProperties>
</slave>
EOF
 
NODE_SECRET=$(curl -L -s -u "${CONTROLLER_USERNAME}:${CONTROLLER_PASSWORD}" -X GET ${JENKINS_URL}/computer/"${NODE_NAME}"/slave-agent.jnlp | sed "s/.*<application-desc main-class=\"hudson.remoting.jnlp.Main\"><argument>\([a-z0-9]*\).*/\1/")
 
# Start the JNLP agent
java -jar ${NODE_WORKSPACE}/agent.jar -jnlpUrl ${JENKINS_URL}/computer/"${NODE_NAME}"/slave-agent.jnlp -secret ${NODE_SECRET}