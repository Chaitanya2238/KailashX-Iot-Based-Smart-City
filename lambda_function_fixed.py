import json
import boto3  # type: ignore - boto3 is available in AWS Lambda runtime
import base64
from datetime import datetime

# Initialize the AWS clients for Rekognition and IoT
rekognition_client = boto3.client('rekognition', region_name='us-east-1')
iot_client = boto3.client('iot-data', region_name='us-east-1')

# The MQTT topic we will publish the command to
COMMAND_TOPIC = 'esp32/commands'

def lambda_handler(event, context):
    print("=== Lambda Function Started ===")
    print(f"Event keys: {event.keys()}")
    
    try:
        # Check if body exists
        if 'body' not in event:
            print("ERROR: No 'body' field in event")
            return create_error_response(400, 'No image data in request body')
        
        # Handle binary data from ESP32
        image_bytes = extract_image_bytes(event)
        
        if not image_bytes or len(image_bytes) < 1000:
            print("ERROR: Image data too small or invalid")
            return create_error_response(400, 'Image data too small or corrupted')
        
        print(f"✓ Image received: {len(image_bytes)} bytes")
        
        # Validate JPEG format
        if not is_valid_jpeg(image_bytes):
            print("ERROR: Invalid JPEG format")
            return create_error_response(400, 'Invalid JPEG image format')
        
        print("✓ Valid JPEG image confirmed")
        
        # Call Rekognition to detect labels
        print("Analyzing image with AWS Rekognition...")
        rekognition_response = rekognition_client.detect_labels(
            Image={'Bytes': image_bytes},
            MaxLabels=20,  # Get more labels for better context
            MinConfidence=80.0,  # HIGHER confidence threshold (was 75)
            Features=['GENERAL_LABELS'],
            Settings={
                'GeneralLabels': {
                    'LabelInclusionFilters': [],
                    'LabelExclusionFilters': []
                }
            }
        )
        
        labels = rekognition_response.get('Labels', [])
        print(f"✓ Rekognition detected {len(labels)} labels")
        
        # Log all detected labels for debugging
        for label in labels[:10]:  # Top 10 labels
            print(f"  - {label['Name']}: {label['Confidence']:.2f}%")
        
        # Check for emergency vehicles with strict matching
        emergency_result = detect_emergency_vehicle(labels)
        
        if emergency_result['is_emergency']:
            print(f"🚨 EMERGENCY VEHICLE DETECTED: {emergency_result['vehicle_type']} ({emergency_result['confidence']:.2f}%)")
            
            # Send MQTT command to ESP32
            send_emergency_command(emergency_result)
            
            return {
                'statusCode': 200,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'emergency_detected',
                    'vehicle_type': emergency_result['vehicle_type'],
                    'confidence': round(emergency_result['confidence'], 2),
                    'timestamp': datetime.now().isoformat(),
                    'message': f"{emergency_result['vehicle_type']} detected with {emergency_result['confidence']:.1f}% confidence"
                })
            }
        else:
            print("✓ No emergency vehicle detected")
            detected_objects = [label['Name'] for label in labels[:5]]
            
            return {
                'statusCode': 200,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'normal',
                    'detected_objects': detected_objects,
                    'timestamp': datetime.now().isoformat(),
                    'message': 'Normal traffic - No emergency vehicle'
                })
            }
        
    except rekognition_client.exceptions.InvalidImageFormatException as e:
        print(f"ERROR: Invalid image format - {str(e)}")
        return create_error_response(400, 'Invalid image format - Must be valid JPEG')
    
    except rekognition_client.exceptions.ImageTooLargeException as e:
        print(f"ERROR: Image too large - {str(e)}")
        return create_error_response(400, 'Image too large - Must be less than 5MB')
    
    except Exception as e:
        print(f"ERROR: Unexpected error - {str(e)}")
        import traceback
        print(traceback.format_exc())
        return create_error_response(500, f'Internal server error: {str(e)}')


def extract_image_bytes(event):
    """Extract image bytes from event, handling different formats"""
    try:
        body = event['body']
        
        # Handle base64 encoded data
        if event.get('isBase64Encoded', False):
            print("Decoding base64 image data")
            return base64.b64decode(body)
        
        # Handle raw binary
        if isinstance(body, bytes):
            print("Using raw binary image data (bytes)")
            return body
        
        # Handle string encoded binary
        if isinstance(body, str):
            print("Using raw binary image data (string)")
            return body.encode('latin1')
        
        return None
    except Exception as e:
        print(f"ERROR extracting image bytes: {str(e)}")
        return None


def is_valid_jpeg(image_bytes):
    """Validate JPEG file format by checking header"""
    try:
        # JPEG files start with FF D8 FF
        if len(image_bytes) < 10:
            return False
        
        # Check JPEG magic number
        if image_bytes[0:2] == b'\xff\xd8':
            print(f"✓ Valid JPEG header detected (FF D8)")
            return True
        
        # Try to find JPEG start marker in case of padding
        jpeg_start = image_bytes.find(b'\xff\xd8\xff')
        if jpeg_start >= 0 and jpeg_start < 100:
            print(f"✓ JPEG marker found at position {jpeg_start}")
            return True
        
        print(f"✗ Invalid JPEG - First bytes: {image_bytes[:10].hex()}")
        return False
        
    except Exception as e:
        print(f"ERROR validating JPEG: {str(e)}")
        return False


def detect_emergency_vehicle(labels):
    """
    Detect emergency vehicles - OPTIMIZED FOR MOBILE SCREENS
    More lenient thresholds for screen-displayed images
    Returns: dict with is_emergency, vehicle_type, confidence
    """
    # MOBILE SCREEN OPTIMIZED - Lower thresholds
    emergency_keywords = {
        'Ambulance': {'priority': 1.0, 'min_confidence': 70.0},  # Was 85
        'Fire Truck': {'priority': 1.0, 'min_confidence': 70.0},  # Was 85
        'Firetruck': {'priority': 1.0, 'min_confidence': 70.0},  # Was 85
        'Police Car': {'priority': 1.0, 'min_confidence': 70.0},  # Was 85
        'Emergency Vehicle': {'priority': 1.0, 'min_confidence': 70.0},  # Was 85
        'Police': {'priority': 0.7, 'min_confidence': 80.0},  # Was 92
        'Emergency': {'priority': 0.6, 'min_confidence': 85.0},  # Was 95
        'Paramedic': {'priority': 0.9, 'min_confidence': 75.0},  # Was 88
        'Fire Engine': {'priority': 1.0, 'min_confidence': 70.0},  # NEW
        'Rescue': {'priority': 0.8, 'min_confidence': 75.0}  # NEW
    }
    
    # Context labels - more lenient for screens
    support_labels = ['Vehicle', 'Car', 'Truck', 'Transportation', 'Automobile', 'Screen', 'Electronics', 'Display']
    has_vehicle_context = False
    
    # Check for vehicle OR screen context
    for label in labels:
        if label['Name'] in support_labels and label['Confidence'] >= 65.0:  # Was 80
            has_vehicle_context = True
            print(f"  ✓ Context found: {label['Name']} ({label['Confidence']:.2f}%)")
            break
    
    best_match = None
    highest_score = 0
    
    for label in labels:
        label_name = label['Name']
        confidence = label['Confidence']
        
        # EXACT keyword matching
        if label_name in emergency_keywords:
            keyword_config = emergency_keywords[label_name]
            priority = keyword_config['priority']
            min_confidence = keyword_config['min_confidence']
            
            score = (confidence * priority) / 100
            
            print(f"  🎯 Match: {label_name} ({confidence:.2f}% / min:{min_confidence:.1f}% / score:{score:.3f})")
            
            # Accept if meets threshold (more lenient for mobile screens)
            if confidence >= min_confidence:
                if priority >= 0.9 or has_vehicle_context:
                    if score > highest_score:
                        highest_score = score
                        best_match = {
                            'is_emergency': True,
                            'vehicle_type': label_name,
                            'confidence': confidence,
                            'detected_label': label_name
                        }
                        print(f"  ✅ EMERGENCY DETECTED!")
                else:
                    print(f"  ❌ REJECTED: No vehicle context found")
            else:
                print(f"  ❌ REJECTED: Confidence {confidence:.2f}% below minimum {min_confidence:.1f}%")
    
    if best_match:
        print(f"  🚨 FINAL DECISION: EMERGENCY VEHICLE = {best_match['vehicle_type']} ({best_match['confidence']:.2f}%)")
        return best_match
    
    print(f"  ✅ FINAL DECISION: NO EMERGENCY VEHICLE")
    return {
        'is_emergency': False,
        'vehicle_type': None,
        'confidence': 0
    }


def send_emergency_command(emergency_result):
    """Send MQTT command to ESP32 via IoT Core"""
    try:
        payload = {
            "command": "EMERGENCY_CLEAR",
            "vehicle_type": emergency_result['vehicle_type'],
            "confidence": round(emergency_result['confidence'], 2),
            "timestamp": datetime.now().isoformat()
        }
        
        print(f"Publishing to MQTT topic: {COMMAND_TOPIC}")
        print(f"Payload: {json.dumps(payload)}")
        
        response = iot_client.publish(
            topic=COMMAND_TOPIC,
            qos=1,
            payload=json.dumps(payload)
        )
        
        print(f"✓ MQTT publish successful: {response}")
        return True
        
    except Exception as e:
        print(f"ERROR publishing to MQTT: {str(e)}")
        return False


def create_error_response(status_code, message):
    """Create standardized error response"""
    return {
        'statusCode': status_code,
        'headers': {
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': '*'
        },
        'body': json.dumps({
            'error': message,
            'timestamp': datetime.now().isoformat()
        })
    }
