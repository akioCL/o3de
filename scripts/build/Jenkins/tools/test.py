import json
import hmac
import hashlib


def verify_signature(secret, signature, payload):
    computed_hash = hmac.new(secret, payload, hashlib.sha1).hexdigest()
    computed_signature = f'sha256={computed_hash}'
    return hmac.compare_digest(computed_signature, signature)


def lambda_handler(event, context):
    print(event)
    print(context)
    secret = event['secret']
    signature = event['X-Hub-Signature-256']
    payload = event['payload']
    effect = "Deny"
    if verify_signature(secret, signature, payload):
        effect = "Allow"
    response = {
        "principalId": event['authorizationToken'],
        "policyDocument": {
            "Version": "2012-10-17",
            "Statement": [
                {
                    "Action": "execute-api:Invoke",
                    "Effect": effect,
                    "Resource": event['methodArn']
                }
            ]
        },
        "context": {
            "stringKey": "ff",
            "numberKey": "1",
            "booleanKey": "true"
        }
    }
    return response